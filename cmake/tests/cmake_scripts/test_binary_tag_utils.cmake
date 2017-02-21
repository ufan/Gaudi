set(CMAKE_MODULE_PATH .. .)

include(GaudiToolchainMacros)
include(TestMacros)

macro(prepare msg)
message(STATUS "   * ${msg}")
set(BINARY_TAG)
set(ENV{CMTCONFIG})
set(ENV{BINARY_TAG})
endmacro()

macro(test_parsing msg tag)
  prepare("parsing ${msg} (${tag})")
  parse_binary_tag(BINARY_TAG ${tag})
endmacro()

macro(test_compatible tag)
  prepare("test compatible tags for ${tag}")
  parse_binary_tag(BINARY_TAG ${tag})
  compatible_binary_tags(out)
endmacro()

include(BinaryTagUtils)

message(STATUS "testing parse_binary_tag()")

prepare("from ENV{CMTCONFIG}")
set(ENV{CMTCONFIG} "x86_64-ecc-gcc7-opt")
parse_binary_tag()
assert_strequal(BINARY_TAG "x86_64-ecc-gcc7-opt")

prepare("from ENV{BINARY_TAG}")
set(ENV{BINARY_TAG} "x86_64-ebt-gcc7-opt")
parse_binary_tag()
assert_strequal(BINARY_TAG "x86_64-ebt-gcc7-opt")

prepare("from BINARY_TAG")
set(BINARY_TAG "x86_64-bt-gcc7-opt")
parse_binary_tag()
assert_strequal(BINARY_TAG "x86_64-bt-gcc7-opt")

prepare("from MY_VAR")
set(MY_VAR "x86_64-mv-gcc7-opt")
parse_binary_tag(MY_VAR)
assert_strequal(BINARY_TAG "")
assert_strequal(MY_VAR "x86_64-mv-gcc7-opt")

prepare("from MY_VAR + value")
assert_strequal(MY_VAR "x86_64-mv-gcc7-opt")
parse_binary_tag(MY_VAR "x86_64-override-gcc7-opt")
assert_strequal(MY_VAR "x86_64-override-gcc7-opt")

prepare("from BINARY_TAG + value")
set(BINARY_TAG "x86_64-mv-gcc7-opt")
parse_binary_tag(BINARY_TAG "x86_64-override-gcc7-opt")
assert_strequal(BINARY_TAG "x86_64-override-gcc7-opt")

prepare("not defined")
parse_binary_tag()
assert(BINARY_TAG)

test_parsing("plain" "x86_64-centos7-gcc7-opt")
assert_strequal(BINARY_TAG_ARCH         "x86_64"  ) 
assert_strequal(BINARY_TAG_MICROARCH    ""        )
assert_strequal(BINARY_TAG_OS           "centos7" )
assert_strequal(BINARY_TAG_COMP         "gcc7"    )
assert_strequal(BINARY_TAG_COMP_NAME    "gcc"     )
assert_strequal(BINARY_TAG_COMP_VERSION "7"       )
assert_strequal(BINARY_TAG_TYPE         "opt"     )

test_parsing("2 digits comp version" "x86_64-centos7-gcc62-opt")
assert_strequal(BINARY_TAG_ARCH         "x86_64"  ) 
assert_strequal(BINARY_TAG_MICROARCH    ""        )
assert_strequal(BINARY_TAG_OS           "centos7" )
assert_strequal(BINARY_TAG_COMP         "gcc62"   )
assert_strequal(BINARY_TAG_COMP_NAME    "gcc"     )
assert_strequal(BINARY_TAG_COMP_VERSION "6.2"     )
assert_strequal(BINARY_TAG_TYPE         "opt"     )

test_parsing("2 digits intel comp version" "x86_64-centos7-icc17-opt")
assert_strequal(BINARY_TAG_ARCH         "x86_64"  ) 
assert_strequal(BINARY_TAG_MICROARCH    ""        )
assert_strequal(BINARY_TAG_OS           "centos7" )
assert_strequal(BINARY_TAG_COMP         "icc17"   )
assert_strequal(BINARY_TAG_COMP_NAME    "icc"     )
assert_strequal(BINARY_TAG_COMP_VERSION "17"      )
assert_strequal(BINARY_TAG_TYPE         "opt"     )

test_parsing("comp without version" "x86_64-centos7-dummy-opt")
assert_strequal(BINARY_TAG_ARCH         "x86_64"  ) 
assert_strequal(BINARY_TAG_MICROARCH    ""        )
assert_strequal(BINARY_TAG_OS           "centos7" )
assert_strequal(BINARY_TAG_COMP         "dummy"   )
assert_strequal(BINARY_TAG_COMP_NAME    "dummy"   )
assert_strequal(BINARY_TAG_COMP_VERSION ""        )
assert_strequal(BINARY_TAG_TYPE         "opt"     )

test_parsing("microarch" "x86_64+avx2-centos7-gcc7-opt")
assert_strequal(BINARY_TAG_ARCH         "x86_64"  ) 
assert_strequal(BINARY_TAG_MICROARCH    "avx2"    )
assert_strequal(BINARY_TAG_OS           "centos7" )
assert_strequal(BINARY_TAG_COMP         "gcc7"    )
assert_strequal(BINARY_TAG_COMP_NAME    "gcc"     )
assert_strequal(BINARY_TAG_COMP_VERSION "7"       )
assert_strequal(BINARY_TAG_TYPE         "opt"     )


message(STATUS "testing compatible_binary_tags(variable)")

test_compatible("x86_64-centos7-gcc7-opt" )
assert_strequal(out "${BINARY_TAG}")

test_compatible("x86_64-centos7-gcc7-dbg" )
assert_strequal(out "${BINARY_TAG};x86_64-centos7-gcc7-opt")

test_compatible("x86_64-centos7-gcc7-any" )
assert_strequal(out "${BINARY_TAG};x86_64-centos7-gcc7-opt")

test_compatible("x86_64+avx2-centos7-gcc7-opt" )
assert_strequal(out "${BINARY_TAG};x86_64-centos7-gcc7-opt")

test_compatible("x86_64+avx2-centos7-gcc7-dbg" )
assert_strequal(out "${BINARY_TAG};x86_64+avx2-centos7-gcc7-opt;x86_64-centos7-gcc7-dbg;x86_64-centos7-gcc7-opt")
