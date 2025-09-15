# 测试

> 运行测试会自动触发构建.

## 测试指定的 task

```bash
bin/taskrunner examples/task1.json  # 或者其它 task 示例.
```

## 运行所有测试

```bash
make test
```

# 构建
## 环境

- Ubuntu 18.04
- Clang++ 18
- G++ 13

## 编译二进制

```bash
$ make
$ ls bin
taskrunner
```

## 清理

```bash
make clean
```
