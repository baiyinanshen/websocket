#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define IMAGE_PATH "static_site/images/liso_header.png"  // 图片文件路径

// 获取文件大小
long get_file_size(int fd) {
    off_t current_pos = lseek(fd, 0, SEEK_CUR);  // 获取当前文件指针位置
    off_t file_size = lseek(fd, 0, SEEK_END);    // 移动到文件末尾获取文件大小
    lseek(fd, current_pos, SEEK_SET);            // 恢复到原来的位置
    return file_size;
}

// 读取文件内容到动态分配的内存中
unsigned char* read_image(const char* filename, long* size) {
    int fd = open(filename, O_RDONLY);  // 以只读模式打开文件
    if (fd == -1) {
        perror("Unable to open image file");
        return NULL;
    }

    // 获取文件大小
    *size = get_file_size(fd);

    // 分配足够大的内存
    unsigned char* buffer = (unsigned char*)malloc(*size);
    if (buffer == NULL) {
        perror("Unable to allocate memory for image");
        close(fd);
        return NULL;
    }

    // 使用 read() 逐块读取文件数据
    ssize_t total_read = 0;
    while (total_read < *size) {
        ssize_t bytes_read = read(fd, buffer + total_read, *size - total_read);
        if (bytes_read == -1) {
            perror("Error reading file");
            free(buffer);
            close(fd);
            return NULL;
        }
        total_read += bytes_read;
    }

    close(fd);  // 关闭文件
    return buffer;
}

int main() {
    long image_size;
    char* image_data = read_image(IMAGE_PATH, &image_size);
    
    if (image_data == NULL) {
        return 1;  // 读取失败
    }
            //printf("Image size: %ld bytes\n", image_size);
            // 使用 image_data 数据进行其他操作，比如发送给客户端、保存到文件等
            // 使用完毕后，释放分配的内存
    //if(levelsend(image_size, buf, client_sock, sock, image_data, sort_file)){return 1;};
    free(image_data);
    return 0;

    return 0;
}
