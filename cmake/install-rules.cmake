install(
    TARGETS tsv-search_exe
    RUNTIME COMPONENT tsv-search_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
