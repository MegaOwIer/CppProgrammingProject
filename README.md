# CppProgrammingProject

## Abstract

Homework for Advanced Programming II, 2020, Renmin University.

A simple defect detection tool for C/C++ with GUI based on LLVM IR and [Apriori Algorithm](https://en.wikipedia.org/wiki/Apriori_algorithm).

## Notice

+ (2020.06.02) Remenber to modify `Makefile` when necessary before you push your work.
+ (2020.06.02) Please DO NOT use quotation marks(`""`) to include headerfiles in llvm library, use angle brackets(`<>`) instead.
+ (2020.05.06) Please format the code with style defined in `.clang-format` before committing.
  + For VS-Code users, you can simply press `Shift+Alt+F` to format the code.
  + If it doesn't work, you can try to append `"C_Cpp.clang_format_style": "file"` after your workspace setting file.
+ (2020.05.06) Please DO NOT commit your local test scripts to the repo. Put then in the folder `test/` instead.

## 进度列表

+ [x] 程序切片与规范化
  + [x] (By hxt) 构建依赖、共享关系
  + [x] (By hxt) 规范化
  + [x] (By wcr) 字符串 hash
+ [x] (By llj & xq) 项集挖掘
+ [x] (By wcr & xq) 规则生成
+ [x] (By wcr) 缺陷检测
+ [x] (By hxt) 图形化界面
