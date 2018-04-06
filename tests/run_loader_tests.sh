#!/bin/bash

pushd $(dirname "$0") > /dev/null

RunEnumerateInstanceExtensionPropertiesTest()
{
    count=$(GTEST_FILTER=EnumerateInstanceExtensionProperties.Count \
        ./vk_loader_validation_tests count 2>&1 |
        grep -o 'count=[0-9]\+' | sed 's/^.*=//')

    if [ "$count" -gt 1 ]
    then
        diff \
            <(GTEST_PRINT_TIME=0 \
                GTEST_FILTER=EnumerateInstanceExtensionProperties.OnePass \
                ./vk_loader_validation_tests count "$count" properties 2>&1 |
                grep 'properties') \
            <(GTEST_PRINT_TIME=0 \
                GTEST_FILTER=EnumerateInstanceExtensionProperties.TwoPass \
                ./vk_loader_validation_tests properties 2>&1 |
                grep 'properties')
    fi
    ec=$?

    if [ $ec -eq 1 ]
    then
        echo "EnumerateInstanceExtensionProperties OnePass vs TwoPass test FAILED - properties do not match" >&2
        exit 1
    fi
    echo "EnumerateInstanceExtensionProperties OnePass vs TwoPass test PASSED"
}

./vk_loader_validation_tests

RunEnumerateInstanceExtensionPropertiesTest

popd > /dev/null
