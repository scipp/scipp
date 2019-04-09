if [ ! -z ${CLANGFORMAT+x} ] && [ "$CLANGFORMAT" == "ON" ]; then

    find core scippy test units -type f -name '*.h' -o -name '*.cpp' \
    | xargs -I{} clang-format-5.0 -i -style=file {};

    dirty=$(git ls-files --modified);

    if [[ $dirty ]]; then
        echo "Clang format FAILED on the following files:";
        echo $dirty;
        exit 1;
    else
        echo "Clang format [ OK ]";
        exit 0;
    fi
fi