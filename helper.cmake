set(URL $ENV{YTDLP_URL})
set(DEST $ENV{YTDLP_DEST})

if(NOT EXISTS "${DEST}")
    file(DOWNLOAD "${URL}" "${DEST}" SHOW_PROGRESS)
    if(NOT WIN32)
        execute_process(COMMAND chmod +x "${DEST}")
    endif()
endif()