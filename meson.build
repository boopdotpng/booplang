project('boop', 'c', version: '0.0.1', default_options: ['warning_level=3'])

cc = meson.get_compiler('c')

# list source files
src_files = files(
    'src/ast.c', 'src/codegen.c', 'src/intern.c', 'src/ir.c', 
    'src/lexer.c', 'src/main.c', 'src/parser.c', 'src/trie.c', 
    'src/utils.c', 'src/vector.c'
)

inc_dir = include_directories('include')

# define executable
executable('boop', src_files, include_directories: inc_dir)