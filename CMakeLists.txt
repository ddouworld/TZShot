cmake_minimum_required(VERSION 3.16)

project(TZshot VERSION 1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(ExternalProject)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Quick
    Qml
    Network
    Concurrent
    LinguistTools
    QuickDialogs2
)

qt_standard_project_setup()

# Help Qt Creator's QML code model resolve local qmldir-based modules
# such as `import TZControls 1.0`.
set(QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml" CACHE STRING "Local QML import paths" FORCE)
set(QML2_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml" CACHE STRING "Local QML import paths" FORCE)

# ── Third-party build options ─────────────────────────────────
option(BUILD_THIRDPART_OCR "Auto build OCR libs" ON)
option(OCR_FETCH_FROM_GITHUB "Fetch Leptonica/Tesseract from GitHub before building" ON)

# ── 翻译文件 ──────────────────────────────────────────────
set(TS_FILES
    i18n/app_zh_CN.ts
    i18n/app_en.ts
)

# ── 平台无关源文件 ────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/tray_icon_helper.cpp
    src/tray_icon_helper.h
    src/language_manager.cpp
    src/language_manager.h
    src/magnifier_provider.cpp
    src/magnifier_provider.h
    src/sticky_image_store.cpp
    src/sticky_image_store.h
    src/sticky_image_provider_proxy.cpp
    src/sticky_image_provider_proxy.h
    src/instance_activation.cpp
    src/instance_activation.h
    src/singleton.h

    # Model
    src/model/desktop_snapshot.cpp
    src/model/desktop_snapshot.h
    src/model/app_settings.cpp
    src/model/app_settings.h

    # ViewModel
    src/viewmodel/screenshot_view_model.cpp
    src/viewmodel/screenshot_view_model.h
    src/viewmodel/sticky_view_model.cpp
    src/viewmodel/sticky_view_model.h
    src/viewmodel/ai_view_model.cpp
    src/viewmodel/ai_view_model.h
    src/viewmodel/storage_view_model.cpp
    src/viewmodel/storage_view_model.h
    src/viewmodel/gif_record_view_model.cpp
    src/viewmodel/gif_record_view_model.h
    src/viewmodel/scroll_capture_view_model.cpp
    src/viewmodel/scroll_capture_view_model.h
    src/viewmodel/ocr_view_model.cpp
    src/viewmodel/ocr_view_model.h

    # GIF Encoder
    src/gif_encoder/gif.h
    src/gif_encoder/gif_encoder.cpp
    src/gif_encoder/gif_encoder.h
    src/gif_encoder/gif_frame_grabber.cpp
    src/gif_encoder/gif_frame_grabber.h
    src/scroll/scroll_stitcher.cpp
    src/scroll/scroll_stitcher.h

    # OCR
    src/ocr/ocr_engine.cpp
    src/ocr/ocr_engine.h

    # PaintBoard
    src/paint_board/paint_board.cpp
    src/paint_board/paint_board.h
    src/paint_board/shape/shape.cpp
    src/paint_board/shape/shape.h
    src/paint_board/shape/shape_type.h
    src/paint_board/shape/shape_factory.cpp
    src/paint_board/shape/shape_factory.h
    src/paint_board/shape/two_point_shape.cpp
    src/paint_board/shape/two_point_shape.h
    src/paint_board/shape/arrow_shape.cpp
    src/paint_board/shape/arrow_shape.h
    src/paint_board/shape/ellipse_shape.cpp
    src/paint_board/shape/ellipse_shape.h
    src/paint_board/shape/mosaic_shape.cpp
    src/paint_board/shape/mosaic_shape.h
    src/paint_board/shape/pen_shape.cpp
    src/paint_board/shape/pen_shape.h
    src/paint_board/shape/rect_shape.cpp
    src/paint_board/shape/rect_shape.h
    src/paint_board/shape/text_shape.cpp
    src/paint_board/shape/text_shape.h
    src/paint_board/shape/number_shape.cpp
    src/paint_board/shape/number_shape.h

    # AI
    src/ai_call/ai_call_base.cpp
    src/ai_call/ai_call_base.h
    src/ai_call/ai_type.h
    src/ai_call/qwen_ai_call.cpp
    src/ai_call/qwen_ai_call.h
    src/ai_call/seedream_ai_call.cpp
    src/ai_call/seedream_ai_call.h

    # Shortcut — 跨平台公共部分
    src/shortcut_key/globalshortcut.cpp
    src/shortcut_key/globalshortcut.h

    # Resources
    qml.qrc
)

# ── 平台专属源文件 ────────────────────────────────────────
if(UNIX)
    list(APPEND SOURCES
        src/shortcut_key/linux_grab.c
        src/shortcut_key/linux_grab.h
    )
endif()

if(WIN32)
    list(APPEND SOURCES
        src/shortcut_key/win_grab.c
        src/shortcut_key/win_grab.h
    )
endif()

qt_add_executable(TZshot
    ${SOURCES}
)

set_target_properties(TZshot PROPERTIES
    QT_QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml"
)

# ── include 路径 ──────────────────────────────────────────
target_include_directories(TZshot PRIVATE
    src
    src/ai_call
    src/gif_encoder
    src/ocr
    src/paint_board
    src/paint_board/shape
    src/model
    src/viewmodel
    src/scroll
    src/shortcut_key
)

# ── 编译定义 ──────────────────────────────────────────────
target_compile_definitions(TZshot PRIVATE
    QT_DEPRECATED_WARNINGS
)

target_compile_definitions(TZshot PRIVATE
    $<$<CONFIG:Debug>:STICKY_MEM_DEBUG>
)

# ── Qt 模块链接 ───────────────────────────────────────────
target_link_libraries(TZshot PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
    Qt6::Concurrent
    Qt6::QuickDialogs2
)

# ── 平台系统库 ────────────────────────────────────────────
if(UNIX)
    target_link_libraries(TZshot PRIVATE
        X11
        xcb
        pthread
    )

    set(OCR_ENABLED FALSE)
    set(OCR_PREFIX "$ENV{OCR_PREFIX}")
    if(OCR_PREFIX STREQUAL "")
        set(OCR_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/thirdpart/ocr")
    endif()

    set(OCR_LIB_DIR "")
    if(EXISTS "${OCR_PREFIX}/lib/libtesseract.so" OR EXISTS "${OCR_PREFIX}/lib/libtesseract.a")
        set(OCR_LIB_DIR "${OCR_PREFIX}/lib")
    elseif(EXISTS "${OCR_PREFIX}/lib64/libtesseract.so" OR EXISTS "${OCR_PREFIX}/lib64/libtesseract.a")
        set(OCR_LIB_DIR "${OCR_PREFIX}/lib64")
    endif()

    if(NOT OCR_LIB_DIR STREQUAL "")
        target_include_directories(TZshot PRIVATE
            "${OCR_PREFIX}/include"
            "${OCR_PREFIX}/include/leptonica"
        )
        target_link_directories(TZshot PRIVATE "${OCR_LIB_DIR}")
        target_link_libraries(TZshot PRIVATE tesseract leptonica)
        target_compile_definitions(TZshot PRIVATE
            ENABLE_OCR
            OCR_TESSDATA_DIR=\"${OCR_PREFIX}/share/tessdata\"
        )
        set(OCR_ENABLED TRUE)
        message(STATUS "OCR enabled via thirdpart: ${OCR_PREFIX}")
    endif()

    if(NOT OCR_ENABLED)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(TESSERACT QUIET IMPORTED_TARGET tesseract)
            pkg_check_modules(LEPT QUIET IMPORTED_TARGET lept)
            if(TESSERACT_FOUND AND LEPT_FOUND)
                target_link_libraries(TZshot PRIVATE PkgConfig::TESSERACT PkgConfig::LEPT)
                target_compile_definitions(TZshot PRIVATE ENABLE_OCR)
                message(STATUS "OCR enabled via pkg-config: tesseract + lept")
            else()
                message(STATUS "OCR disabled: thirdpart OCR not found and pkg-config tesseract/lept unavailable")
            endif()
        else()
            message(STATUS "OCR disabled: PkgConfig not found")
        endif()
    endif()
endif()

if(WIN32)
    target_link_libraries(TZshot PRIVATE
        user32   # RegisterHotKey / UnregisterHotKey / EnumWindows
        gdi32
    )

    set(OCR_ENABLED FALSE)
    if(BUILD_THIRDPART_OCR AND OCR_FETCH_FROM_GITHUB)
        set(OCR_THIRDPART_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdpart")
        set(OCR_INSTALL_DIR "${OCR_THIRDPART_DIR}/ocr-install")
        set(OCR_TESSDATA_DIR "${OCR_INSTALL_DIR}/share/tessdata")
        # ExternalProject build trees must be generator-specific.
        # Otherwise switching between Ninja/VS reuses incompatible CMake caches.
        set(OCR_EP_KEY "${CMAKE_GENERATOR}-${CMAKE_GENERATOR_PLATFORM}")
        string(REGEX REPLACE "[^A-Za-z0-9_.-]" "_" OCR_EP_KEY "${OCR_EP_KEY}")
        set(OCR_EP_ROOT_DIR "${OCR_THIRDPART_DIR}/_ep_${OCR_EP_KEY}")
        set(OCR_ZLIB_VERSION "v1.3.1")
        set(OCR_LIBPNG_VERSION "v1.6.43")
        set(OCR_LIBJPEG_TURBO_VERSION "3.0.3")
        set(OCR_LIBTIFF_VERSION "v4.7.0")
        set(OCR_LEPTONICA_VERSION "1.87.0")
        set(OCR_TESSERACT_VERSION "5.5.2")
        # Imported target include dirs are validated at generate time.
        # Create expected install dirs up front; actual contents are produced by ExternalProject.
        file(MAKE_DIRECTORY
            "${OCR_INSTALL_DIR}/include"
            "${OCR_INSTALL_DIR}/lib"
            "${OCR_TESSDATA_DIR}"
        )

        # Make ExternalProject follow current build type (Debug/Release) on
        # single-config generators (e.g. Ninja in Qt Creator kits).
        set(OCR_EP_BUILD_TYPE_ARG "")
        if(NOT CMAKE_CONFIGURATION_TYPES)
            if(CMAKE_BUILD_TYPE STREQUAL "")
                set(OCR_EP_BUILD_TYPE "Debug")
            else()
                set(OCR_EP_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
            endif()
            set(OCR_EP_BUILD_TYPE_ARG "-DCMAKE_BUILD_TYPE=${OCR_EP_BUILD_TYPE}")
        endif()

        ExternalProject_Add(zlib_ep
            PREFIX "${OCR_EP_ROOT_DIR}/zlib"
            GIT_REPOSITORY "https://github.com/madler/zlib.git"
            GIT_TAG "${OCR_ZLIB_VERSION}"
            UPDATE_DISCONNECTED 1
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${OCR_INSTALL_DIR}
                -DBUILD_SHARED_LIBS=ON
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                ${OCR_EP_BUILD_TYPE_ARG}
        )

        ExternalProject_Add(libpng_ep
            PREFIX "${OCR_EP_ROOT_DIR}/libpng"
            GIT_REPOSITORY "https://github.com/pnggroup/libpng.git"
            GIT_TAG "${OCR_LIBPNG_VERSION}"
            UPDATE_DISCONNECTED 1
            DEPENDS zlib_ep
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${OCR_INSTALL_DIR}
                -DCMAKE_PREFIX_PATH=${OCR_INSTALL_DIR}
                -DZLIB_ROOT=${OCR_INSTALL_DIR}
                -DPNG_SHARED=ON
                -DPNG_TESTS=OFF
                -DPNG_EXECUTABLES=OFF
                -DPNG_FRAMEWORK=OFF
                ${OCR_EP_BUILD_TYPE_ARG}
        )

        ExternalProject_Add(libjpeg_ep
            PREFIX "${OCR_EP_ROOT_DIR}/libjpeg"
            GIT_REPOSITORY "https://github.com/libjpeg-turbo/libjpeg-turbo.git"
            GIT_TAG "${OCR_LIBJPEG_TURBO_VERSION}"
            UPDATE_DISCONNECTED 1
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${OCR_INSTALL_DIR}
                -DBUILD_SHARED_LIBS=ON
                -DENABLE_SHARED=ON
                -DENABLE_STATIC=OFF
                -DWITH_TURBOJPEG=ON
                -DWITH_JPEG8=ON
                ${OCR_EP_BUILD_TYPE_ARG}
        )

        ExternalProject_Add(libtiff_ep
            PREFIX "${OCR_EP_ROOT_DIR}/libtiff"
            GIT_REPOSITORY "https://github.com/libsdl-org/libtiff.git"
            GIT_TAG "${OCR_LIBTIFF_VERSION}"
            UPDATE_DISCONNECTED 1
            DEPENDS zlib_ep
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${OCR_INSTALL_DIR}
                -DCMAKE_PREFIX_PATH=${OCR_INSTALL_DIR}
                -DBUILD_SHARED_LIBS=ON
                -Dzlib=ON
                -Djpeg=OFF
                -Dwebp=OFF
                -Dzstd=OFF
                -Dlzma=OFF
                -Djbig=OFF
                -Dlerc=OFF
                -Dtiff-tools=OFF
                -Dtiff-tests=OFF
                -Dtiff-contrib=OFF
                -Dtiff-docs=OFF
                ${OCR_EP_BUILD_TYPE_ARG}
        )

        ExternalProject_Add(leptonica_ep
            PREFIX "${OCR_EP_ROOT_DIR}/leptonica"
            GIT_REPOSITORY "https://github.com/DanBloomberg/leptonica.git"
            GIT_TAG "${OCR_LEPTONICA_VERSION}"
            UPDATE_DISCONNECTED 1
            DEPENDS libpng_ep libjpeg_ep libtiff_ep
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${OCR_INSTALL_DIR}
                -DCMAKE_PREFIX_PATH=${OCR_INSTALL_DIR}
                -DBUILD_SHARED_LIBS=ON
                -DCMAKE_POSITION_INDEPENDENT_CODE=ON
                -DSW_BUILD=OFF
                -DSTRICT_CONF=ON
                -DENABLE_ZLIB=ON
                -DENABLE_PNG=ON
                -DENABLE_GIF=OFF
                -DENABLE_JPEG=ON
                -DENABLE_TIFF=ON
                -DENABLE_WEBP=OFF
                -DENABLE_OPENJPEG=OFF
                ${OCR_EP_BUILD_TYPE_ARG}
        )

        ExternalProject_Add(tesseract_ep
            PREFIX "${OCR_EP_ROOT_DIR}/tesseract"
            GIT_REPOSITORY "https://github.com/tesseract-ocr/tesseract.git"
            GIT_TAG "${OCR_TESSERACT_VERSION}"
            UPDATE_DISCONNECTED 1
            DEPENDS leptonica_ep
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${OCR_INSTALL_DIR}
                -DCMAKE_PREFIX_PATH=${OCR_INSTALL_DIR}
                -DBUILD_SHARED_LIBS=ON
                -DSW_BUILD=OFF
                -DBUILD_TRAINING_TOOLS=OFF
                -DBUILD_TESTS=OFF
                -DDISABLE_ARCHIVE=ON
                -DDISABLE_CURL=ON
                -DOPENMP_BUILD=OFF
                ${OCR_EP_BUILD_TYPE_ARG}
        )

        add_dependencies(TZshot tesseract_ep)

        target_include_directories(TZshot PRIVATE "${OCR_INSTALL_DIR}/include")
        target_link_directories(TZshot PRIVATE "${OCR_INSTALL_DIR}/lib")
        set(OCR_LEPTONICA_LINK_NAME "leptonica-${OCR_LEPTONICA_VERSION}$<$<CONFIG:Debug>:d>")
        if(MINGW)
            # Our MinGW Leptonica ExternalProject installs libleptonica.dll.a
            # without a version/debug suffix, so link against that stable name.
            set(OCR_LEPTONICA_LINK_NAME "leptonica")
        endif()
        target_link_libraries(TZshot PRIVATE
            "tesseract55$<$<CONFIG:Debug>:d>"
            "${OCR_LEPTONICA_LINK_NAME}"
            Ws2_32
        )

        target_compile_definitions(TZshot PRIVATE
            ENABLE_OCR
            OCR_TESSDATA_DIR=\"${OCR_TESSDATA_DIR}\"
        )

        # Ensure runtime OCR DLLs are available beside TZshot.exe.
        add_custom_command(TARGET TZshot POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_directory
                    "${OCR_INSTALL_DIR}/bin"
                    "$<TARGET_FILE_DIR:TZshot>"
            VERBATIM
        )

        set(OCR_ENABLED TRUE)
        message(STATUS "OCR enabled via GitHub fetch/build (Leptonica ${OCR_LEPTONICA_VERSION}, Tesseract ${OCR_TESSERACT_VERSION})")
    endif()

    if(NOT OCR_ENABLED)
        message(STATUS "OCR disabled on Windows: BUILD_THIRDPART_OCR/OCR_FETCH_FROM_GITHUB is OFF")
    endif()

    # Windows 子系统：Release 用 /SUBSYSTEM:WINDOWS 隐藏控制台窗口
    set_target_properties(TZshot PROPERTIES
        WIN32_EXECUTABLE $<IF:$<CONFIG:Debug>,FALSE,TRUE>
    )
endif()

# ── QML 导入路径 ──────────────────────────────────────────
set_target_properties(TZshot PROPERTIES
    QT_QML_IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/qml"
)

# ── 翻译 ──────────────────────────────────────────────────
qt_add_translations(TZshot
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

# Package generated .qm files into the app resource at :/i18n.
foreach(qm_file IN LISTS QM_FILES)
    get_filename_component(qm_name "${qm_file}" NAME)
    set_source_files_properties("${qm_file}" PROPERTIES
        QT_RESOURCE_ALIAS "${qm_name}"
    )
endforeach()

qt_add_resources(TZshot "i18n_translations"
    PREFIX "/i18n"
    FILES ${QM_FILES}
)

# ── 安装 ──────────────────────────────────────────────────
if(UNIX AND NOT ANDROID)
    install(TARGETS TZshot
        DESTINATION /opt/TZshot/bin
    )
endif()

if(WIN32)
    install(TARGETS TZshot
        DESTINATION bin
    )
endif()
