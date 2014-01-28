require 'mkmf'

dir_config('mecab')
unless have_header('mecab.h') && have_library('mecab', 'mecab_new')
  abort "header files and library of mecab are required"
end

have_func('mecab_model_new', %[mecab.h])

create_makefile('mecaby/mecaby')
