#pragma once

enum class FileErrorCode {
    OK = 0,
    NotFound = 101,
    OpenFailure = 102,
    WriteFailure = 103,
    ReadFailure = 104,
    DeleteFailure = 105,
    SeekFailure = 106,
};
