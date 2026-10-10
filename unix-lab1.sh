#! /bin/sh   
if [ -f "$1" ]; then
    echo "Файл существует"
else
    echo "Файла нет" >&2
    exit 1
fi

case "$1" in
    *.c)
        echo "Файл поддерживаемого формата"
        ;;
    *)
        echo "Файл не поддерживаемого формата (требуется .c)" >&2
        exit 2
        ;;
esac

output_name=$(grep ".*//.*[Oo]utput:" "$1" | sed 's/.*[Oo]utput:[[:space:]]*//' | head -n 1 )

case "$output_name" in
    */*|.|..)
        echo "Некорректное имя файла $output_name" >&2
        exit 3
        ;;
esac

if [ -n "$output_name" ]; then
    echo "Имя output файла : $output_name"
else
    echo "Не найдено имя output файла" >&2
    exit 4
fi

if ! tmp_dir=$(mktemp -d); then 
    echo "Ошибка создания временной директории" >&2
    exit 5
else
    echo "Создана временная директория $tmp_dir"
fi

delete(){
    rm -rf "$tmp_dir"
    echo "Временные файлы удалены"
}

trap 'delete' 0
trap 'exit 130' INT
trap 'exit 143' TERM
trap 'exit 129' HUP

source_dir=$(cd "$(dirname "$1")" && pwd)
source_name=$(basename "$1")

if ! (cd "$tmp_dir" && gcc "$source_dir/$source_name" -o "$output_name"); then
    echo "Ошибка компиляции" >&2
    exit 6
else
    echo "Компиляция прошла успешно" 
fi   

if ! mv "$tmp_dir/$output_name" "$source_dir/$output_name"; then
    echo "Файл не удалось переместить" >&2
    exit 7
else
    echo "Файл перемещен в $source_dir" 
fi   
