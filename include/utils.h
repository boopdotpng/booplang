#pragma once

char *read_file_to_buffer(const char *filename);

int write_buffer_to_file(const char *filename, const char *buffer);

long get_file_size(const char *filename);