#include "tools.h"

int atomic_write_file(const char* filename, const char* data, size_t size) {
    int fd = open(filename, O_WRONLY | O_CREAT, 0664);
    if (fd == -1) {
        return errno;
    }

    // Заблокировать файл для записи
    if (flock(fd, LOCK_EX) == -1) {
        close(fd);
        return errno;
    }

    // Записать данные
    if (write(fd, data, size) != size) {
        flock(fd, LOCK_UN);
        close(fd);
        return EIO;
    }

    // Разблокировать файл
    if (flock(fd, LOCK_UN) == -1) {
        close(fd);
        return errno;
    }

    close(fd);
    return 0;
}

int atomic_read_file(const char* filename, char* data, size_t size) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return errno;
    }

    // lock for write data
    if (flock(fd, LOCK_SH) == -1) {
        close(fd);
        return errno;
    }

    ssize_t read_size = read(fd, data, size);
    if (read_size < 0) {
        flock(fd, LOCK_UN);
        close(fd);
        return EIO;
    }

    if (flock(fd, LOCK_UN) == -1) {
        close(fd);
        return errno;
    }

    close(fd);
    data[read_size] = '\0';
    return 0;
}

int init_array(DynArray *array, int cap) {
    array->data = calloc(cap, sizeof(*(array->data)));
    if (!array->data) {
        return -1;
    }
    array->size = 0;
    array->capacity = cap;
    return 0;
}

int add_elem_to_array(DynArray *array, int elem) {
    if (array->size + 1 > array->capacity) {
        size_t newcap = 2 * (array->capacity + 1);
        int *tmp = realloc(array->data, newcap * sizeof(*tmp));
        if (!tmp) {
            free(array->data);
            return -1;
        }
        array->data = tmp;
        array->capacity = newcap;
    }
    array->data[array->size++] = elem;
    return 0;
}