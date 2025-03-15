#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <aio.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <algorithm>
#include <iostream>
#include <vector>

using namespace std;

/*
 * Структура aio_operation описывает одну операцию ввода-вывода:
 * - aio: структура aiocb, содержащая параметры асинхронной операции.
 * - buffer: указатель на выделенный буфер, куда читаются данные, либо данные для записи.
 * - offset: смещение (позиция) в файле, с которого начинается операция.
 * - is_write: флаг, показывающий, является ли операция операцией записи (true) или чтения (false).
 */
struct aio_operation {
    struct aiocb aio;
    char *buffer;
    off_t offset;    // Смещение в файле для данной операции
    bool is_write;   // Если false, то операция чтения, если true – операция записи
};

/*
 * Функция launch_read инициализирует операцию чтения:
 * - Выделяет буфер с размером не более block_size или оставшегося числа байт до конца файла.
 * - Заполняет поля структуры aiocb, соответствующие операции чтения.
 * - Запускает асинхронное чтение с помощью aio_read().
 *
 * Параметры:
 *   op         - указатель на структуру операции
 *   read_fd    - файловый дескриптор исходного файла
 *   offset     - смещение в файле, с которого производится чтение
 *   block_size - максимальный размер блока чтения
 *   file_size  - полный размер файла, чтобы не выйти за его пределы
 *
 * Возвращает:
 *   true в случае успеха и false при ошибке.
 */
bool launch_read(aio_operation *op, int read_fd, off_t offset, size_t block_size, off_t file_size) {
    // Вычисляем число байт, которое можно прочитать (не больше, чем осталось от file_size)
    size_t bytes_to_read = min(block_size, (size_t)(file_size - offset));
    op->buffer = (char*)malloc(bytes_to_read);
    if (!op->buffer) {
        perror("Error allocating read buffer");
        return false;
    }
    op->offset = offset;
    op->is_write = false; // Это операция чтения
    // Обнуляем структуру aiocb
    memset(&op->aio, 0, sizeof(struct aiocb));
    op->aio.aio_fildes = read_fd;        // Файловый дескриптор для чтения
    op->aio.aio_buf = op->buffer;          // Буфер для чтения
    op->aio.aio_nbytes = bytes_to_read;    // Размер читаемых данных
    op->aio.aio_offset = offset;           // Смещение в файле
    // Уведомление о завершении операции не используется, поэтому выбираем SIGEV_NONE.
    op->aio.aio_sigevent.sigev_notify = SIGEV_NONE;
    // Запуск асинхронного чтения
    if (aio_read(&op->aio) == -1) {
        perror("Error starting aio_read");
        free(op->buffer);
        return false;
    }
    return true;
}

/*
 * Функция launch_write инициализирует операцию записи:
 * - Переиспользует выделенный ранее буфер (полученный при чтении).
 * - Заполняет поля структуры aiocb для операции записи.
 * - Запускает асинхронную запись с помощью aio_write().
 *
 * Параметры:
 *   op      - указатель на операцию (буфер уже заполнен)
 *   write_fd- файловый дескриптор файла, куда производим запись
 *   bytes   - число байт для записи (результат завершившейся операции aio_read)
 *
 * Возвращает:
 *   true в случае успеха и false при ошибке.
 */
bool launch_write(aio_operation *op, int write_fd, ssize_t bytes) {
    op->is_write = true; // Переключаем флаг на запись
    // Обнуляем структуру aiocb перед тем, как заполнить поля для записи.
    memset(&op->aio, 0, sizeof(struct aiocb));
    op->aio.aio_fildes = write_fd;    // Файловый дескриптор для записи
    op->aio.aio_buf = op->buffer;       // Буфер с данными для записи
    op->aio.aio_nbytes = bytes;         // Количество байт для записи
    op->aio.aio_offset = op->offset;    // Смещение, куда производить запись
    op->aio.aio_sigevent.sigev_notify = SIGEV_NONE;
    // Запуск асинхронной записи
    if (aio_write(&op->aio) == -1) {
        perror("Error starting aio_write");
        return false;
    }
    return true;
}

/*
 * Функция copy_file копирует содержимое файла с использованием асинхронных операций.
 * Она организует пул асинхронных операций, позволяя иметь одновременно max_concurrent
 * операций ввода-вывода. Внутри производится циклическое ожидание завершения операций с
 * использованием aio_suspend().
 *
 * Параметры:
 *   read_filename  - имя исходного файла
 *   write_filename - имя файла назначения
 *   block_size     - размер блока для чтения/записи
 *   max_concurrent - максимальное число одновременно выполняемых асинхронных операций
 *
 * Возвращает:
 *   время работы копирования в секундах.
 */
double copy_file(const char *read_filename, const char *write_filename, size_t block_size, int max_concurrent) {
    // Открытие исходного файла для чтения
    int read_fd = open(read_filename, O_RDONLY | O_NONBLOCK);
    if (read_fd == -1) {
        perror("Error opening source file");
        exit(EXIT_FAILURE);
    }

    // Открытие файла назначения для записи (с созданием/очищением)
    int write_fd = open(write_filename, O_CREAT | O_WRONLY | O_TRUNC | O_NONBLOCK, 0666);
    if (write_fd == -1) {
        perror("Error opening destination file");
        close(read_fd);
        exit(EXIT_FAILURE);
    }

    // Получение размера исходного файла
    struct stat file_stat;
    if (fstat(read_fd, &file_stat) == -1) {
        perror("Error getting file size");
        close(read_fd);
        close(write_fd);
        exit(EXIT_FAILURE);
    }
    off_t file_size = file_stat.st_size;
    off_t current_offset = 0;  // Текущее смещение будем обновлять после каждой операции

    // Создание вектора указателей на операции (размер равен числу одновременно активных операций)
    vector<aio_operation*> slots(max_concurrent, nullptr);

    // Запускаем начальные операции чтения для каждого слота, если ещё остались данные в файле.
    for (int i = 0; i < max_concurrent && current_offset < file_size; i++) {
        aio_operation *op = new aio_operation;
        if (!launch_read(op, read_fd, current_offset, block_size, file_size)) {
            delete op;
            break;
        }
        slots[i] = op;
        // Обновляем смещение: увеличиваем его на число байт, запланированных для чтения
        current_offset += op->aio.aio_nbytes;
    }

    // Фиксируем время начала операции копирования.
    clock_t start = clock();

    // Основной цикл обработки слотов операций.
    // Продолжаем, пока в векторе есть хотя бы одна активная (не NULL) операция.
    while (true) {
        // Собираем список указателей на aiocb всех активных операций для последующего ожидания.
        vector<const struct aiocb*> aiocb_list;
        for (auto op : slots) {
            if (op != nullptr)
                aiocb_list.push_back(&op->aio);
        }
        // Если нет активных операций — значит, все данные обработаны, выходим из цикла.
        if (aiocb_list.empty())
            break;

        // Ожидаем завершения хотя бы одной операции с помощью aio_suspend.
        int ret;
        do {
            ret = aio_suspend(aiocb_list.data(), aiocb_list.size(), nullptr);
        } while (ret == -1 && errno == EINTR);

        // Для каждого слота проверяем, завершилась ли операция.
        for (int i = 0; i < max_concurrent; i++) {
            aio_operation *op = slots[i];
            if (op == nullptr)
                continue;
            // Получаем статус операции: если еще не завершена — пропускаем.
            int err = aio_error(&op->aio);
            if (err == EINPROGRESS)
                continue;
            // Если при выполнении операции произошла ошибка, выводим сообщение и освобождаем слот.
            if (err != 0) {
                fprintf(stderr, "AIO error at offset %ld: %s\n", (long)op->offset, strerror(err));
                free(op->buffer);
                delete op;
                slots[i] = nullptr;
                continue;
            }

            // Получаем число обработанных байт (результат завершенной операции)
            ssize_t bytes = aio_return(&op->aio);
            if (bytes <= 0) {
                // Если достигнут конец файла или произошла ошибка,
                // освобождаем слот и продолжаем.
                free(op->buffer);
                delete op;
                slots[i] = nullptr;
                continue;
            }

            // Если завершилась операция чтения (is_write == false),
            // запускаем операцию записи для полученных данных.
            if (!op->is_write) {
                if (!launch_write(op, write_fd, bytes)) {
                    free(op->buffer);
                    delete op;
                    slots[i] = nullptr;
                    continue;
                }
            } else {
                // Если завершилась операция записи:
                // 1. Освобождаем буфер и удаляем операцию.
                // 2. Запускаем новую операцию чтения в этом слоте, если данные еще остались.
                free(op->buffer);
                delete op;
                slots[i] = nullptr;
                if (current_offset < file_size) {
                    aio_operation *new_op = new aio_operation;
                    if (!launch_read(new_op, read_fd, current_offset, block_size, file_size)) {
                        delete new_op;
                    } else {
                        slots[i] = new_op;
                        // Обновляем смещение для следующего блока
                        current_offset += new_op->aio.aio_nbytes;
                    }
                }
            }
        }
    } // Конец основного цикла обработки операций

    // Фиксируем время завершения операции копирования.
    clock_t end = clock();
    double duration = (double)(end - start) / CLOCKS_PER_SEC;

    close(read_fd);
    close(write_fd);

    return duration;
}

/*
 * Функция main:
 * - Принимает два аргумента: имя исходного файла и имя файла назначения.
 * - Для каждого размера блока (от 1024 до 131072 байт) и каждого уровня перекрытия
 *   (1, 2, 4, 8) вызывается функция copy_file.
 * - Измеряется время выполнения копирования и вычисляется скорость в МБ/с.
 * - Результаты выводятся в виде таблицы.
 */
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <source file> <destination file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *read_filename = argv[1];
    const char *write_filename = argv[2];

    // Массив размеров блоков для тестирования.
    size_t block_sizes[] = {1024, 4096, 8192, 16384, 32768, 65536, 131072};
    int num_block_sizes = sizeof(block_sizes) / sizeof(block_sizes[0]);

    // Массив уровней перекрытия (число одновременно выполняемых операций).
    int concurrent_levels[] = {1, 2, 4, 8, 12, 16};
    int num_concurrent_levels = sizeof(concurrent_levels) / sizeof(concurrent_levels[0]);

    cout << "Block Size (bytes)\tConcurrent Ops\tTime (seconds)\tSpeed (MB/s)" << endl;
    // Внешний цикл по размерам блока.
    for (int i = 0; i < num_block_sizes; i++) {
        size_t block_size = block_sizes[i];
        // Внутренний цикл по числу перекрывающихся операций.
        for (int j = 0; j < num_concurrent_levels; j++) {
            int concurrent = concurrent_levels[j];
            // Для каждого случая запускается копирование. Заметьте, что каждый раз в destination
            // происходит перезапись файла.
            double duration = copy_file(read_filename, write_filename, block_size, concurrent);
            struct stat file_stat;
            stat(read_filename, &file_stat); // Получаем размер файла
            off_t file_size = file_stat.st_size;
            double speed = (file_size / (1024.0 * 1024.0)) / duration;
            printf("%zu\t\t\t%d\t\t%.6f\t%.2f\n", block_size, concurrent, duration, speed);
        }
    }

    return 0;
}