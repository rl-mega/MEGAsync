#ifndef VERSION_H
#define VERSION_H

#define VER_FILEVERSION             4, 9, 4, 0
#define VER_FILEVERSION_CODE        4904
#define VER_PRODUCTVERSION          4, 9, 4, 0
// Update scrips relying on this value if you move it
#define VER_PRODUCTVERSION_STR      "4.9.4.0\0"

#define VER_BUILD_ID                0

#define VER_COMPANYNAME_STR         "Mega Limited\0"
#define VER_FILEDESCRIPTION_STR     "MEGAsync\0"
#define VER_INTERNALNAME_STR        "MEGAsync.exe\0"
#define VER_LEGALCOPYRIGHT_STR      "Mega Limited 2023\0"
#define VER_LEGALTRADEMARKS1_STR    "All Rights Reserved"
#define VER_ORIGINALFILENAME_STR    "MEGAsync.exe\0"
#define VER_PRODUCTNAME_STR         "MEGAsync\0"

#define VER_SDK_ID                  "0f65318"

// Update scrips relying on this value if you move it
// Format: 1 item by line, starting from line following the #define
//#define VER_CHANGES_NOTES QT_TRANSLATE_NOOP("Preferences", \
//"- item 1\n"               \
//"- item 2\n\"              \
// [...]
//"- item n\n"
//)
#define VER_CHANGES_NOTES  QT_TRANSLATE_NOOP("Preferences",                                        \
"- You can now search the stored data in your account.\n"                                         \
"- We've improved system notifications.\n"                                                         \
"- We've enhanced the UI.\n"                                                                       \
"- We've fixed the detected crashes on Windows, Linux, and macOS.\n"                               \
)

#endif // VERSION_H