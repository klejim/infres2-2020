function(configure_file_generate)
    set(intermediate_file_suffix "_cf_only")
    list(GET ARGV 0 input_file)
    list(GET ARGV 1 output_file)
    set(intermediate_file ${output_file}${intermediate_file_suffix})

    list(REMOVE_AT ARGV 1)
    list(INSERT ARGV 1 ${intermediate_file})

    configure_file(${ARGV})

    file(GENERATE OUTPUT ${output_file} INPUT ${intermediate_file})
endfunction()

