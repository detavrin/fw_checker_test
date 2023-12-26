add_executable(app main.c logging.c)

include(binutils-arm-none-eabi)

# Добавить команду вывода размеров секций после сборки
print_section_sizes(app)
# Создать файл прошивки в бинарном формате
create_bin_output(app)