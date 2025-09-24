idf_component_register(SRCS "minishark.c" "display_handler.c" "mac_database.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wunused-function
    -Wunused-variable
    -Wunused-value
    -Os
)
