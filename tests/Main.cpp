//
// Created by Plutex on 2026-10-21.
//
// Entry point for the PluNetwork tests. The runner is PluSTL's (PluSTL/tests/TestFramework.h):
// every test file self-registers through PLU_TEST, and the failure count becomes the exit code.

#include "TestFramework.h"

#include <cstdio>

int main()
{
    std::printf("PluNetwork tests\n");
    std::printf("================\n\n");

    const int failedTests = PluTest::RunAll();
    return failedTests == 0 ? 0 : 1;
}
