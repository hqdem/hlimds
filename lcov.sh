#!/bin/sh

# Removes file/directory if exists
rm_if_exists()
{
    FILE=$1
    if [ -e $FILE ]
    then
        echo "Removing previously generated ${FILE} ..."
        rm -rf ${FILE}
        echo "done!"
    fi
}

BUILD="${UTOPIA_HOME}/build"
OUTPUT="${UTOPIA_HOME}/output"
COV_INFO="${OUTPUT}/coverage.info"
FILTERED_COV="${OUTPUT}/filtered.info"
REPORT_DIR="${OUTPUT}/coverage-report"

rm_if_exists ${COV_INFO}

lcov --capture --directory ${UTOPIA_HOME} --exclude *.y --exclude *.l -o ${COV_INFO}

rm_if_exists ${FILTERED_COV}

lcov --remove ${COV_INFO} "${BUILD}/*" "${UTOPIA_HOME}/lib/*" "/usr/*" -o ${FILTERED_COV}

rm_if_exists ${REPORT_DIR}

genhtml ${FILTERED_COV} -o ${REPORT_DIR}

echo "HTML report has been stored to '${REPORT_DIR}' directory."
