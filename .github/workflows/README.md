# ASR-RK3588 CI/CD 流程说明

## 概述

本项目的 CI/CD 流程使用 GitHub Actions 实现，自动化完成交叉编译构建、验证和发布等流程。

## 工作流触发条件

- **推送到主分支**: `main`、`master`、`develop`
- **Pull Request**: 目标分支为 `main`、`master`、`develop`
- **手动触发**: 在 GitHub Actions 页面手动触发
- **标签推送**: 当推送符合 `refs/tags/*` 格式的标签时，会创建 GitHub Release

## 工作流任务

### 1. 交叉编译构建 (build-cross-compile)

**目的**: 在 Ubuntu 环境中为 RK3588 (aarch64) 平台交叉编译项目

**构建类型**:
- `Release`: 生产版本，优化性能
- `Debug`: 调试版本，包含调试信息

**步骤**:
1. 检出代码
2. 安装交叉编译工具链:
   - `gcc-aarch64-linux-gnu`
   - `g++-aarch64-linux-gnu`
   - `cmake`
   - `make`
3. 验证交叉编译工具链版本
4. 运行 `build.sh` 进行构建:
   - Release 版本: `./build.sh`
   - Debug 版本: `./build.sh 1`
5. 检查构建产物是否存在
6. 上传构建产物（保留30天）

**构建产物**:
- 可执行文件: `install/asr_rknn_serve`
- 运行脚本: `install/run.sh`、`install/stop.sh`
- 资源文件: `install/resources/`
- 库文件: `install/lib/`

### 2. 构建验证 (build-verify)

**目的**: 验证构建产物的完整性和正确性

**触发条件**: 仅在推送到 `main` 或 `master` 分支时执行

**步骤**:
1. 检出代码
2. 下载 Release 版本的构建产物
3. 验证安装目录结构:
   - ✓ 可执行文件存在
   - ✓ run.sh 脚本存在
   - ✓ stop.sh 脚本存在
   - ✓ resources 目录存在
   - ✓ lib 目录存在
4. 输出完整的目录结构

### 3. 创建发布版本 (release)

**目的**: 自动创建 GitHub Release 并上传构建产物

**触发条件**: 仅在推送标签时执行（如 `v1.0.0`）

**步骤**:
1. 检出代码
2. 下载 Release 版本的构建产物
3. 打包为 `tar.gz` 格式
4. 创建 GitHub Release，包含:
   - 打包的构建产物
   - 发布说明
   - 构建信息
   - 使用说明

## 使用指南

### 开发者日常使用

1. **本地开发**:
   ```bash
   # 构建 Release 版本
   ./build.sh
   
   # 构建 Debug 版本
   ./build.sh 1
   ```

2. **提交代码**:
   - 推送到 `develop` 分支进行测试
   - 通过 Pull Request 合并到 `main` 分支

3. **查看构建状态**:
   - 访问 GitHub 仓库的 "Actions" 标签页
   - 查看最新的工作流运行状态
   - 下载构建产物进行测试

### 发布新版本

1. **创建标签**:
   ```bash
   # 创建并推送标签
   git tag -a v1.0.0 -m "Release v1.0.0"
   git push origin v1.0.0
   ```

2. **自动发布**:
   - GitHub Actions 会自动触发发布流程
   - 构建产物会自动打包并上传到 Release
   - Release 页面会自动生成发布说明

3. **验证发布**:
   - 访问 GitHub 仓库的 "Releases" 标签页
   - 下载构建产物测试
   - 确认版本信息正确

### 手动触发构建

1. 访问 GitHub 仓库的 "Actions" 标签页
2. 选择 "ASR-RK3588 CI/CD" 工作流
3. 点击 "Run workflow" 按钮
4. 选择分支并点击 "Run workflow"

## 构建产物

### Artifact 下载

1. 在 Actions 页面找到成功的构建运行
2. 滚动到底部的 "Artifacts" 部分
3. 下载需要的构建产物:
   - `asr-rknn-serve-Release`: Release 版本
   - `asr-rknn-serve-Debug`: Debug 版本

### Release 下载

1. 访问 GitHub 仓库的 "Releases" 标签页
2. 选择对应的版本
3. 下载 `ASR-RK3588-<version>.tar.gz` 文件

## 故障排查

### 构建失败常见原因

1. **交叉编译失败**:
   - 检查 `build.sh` 脚本是否有修改
   - 确认第三方库文件完整
   - 查看构建日志获取详细错误信息

2. **构建产物验证失败**:
   - 检查 CMakeLists.txt 中的安装规则
   - 确认所有必需文件都被正确安装

### 查看详细日志

1. 访问 GitHub Actions 页面
2. 点击失败的构建运行
3. 点击失败的步骤查看详细日志
4. 下载完整的日志文件进行分析

## 配置说明

### 修改构建触发条件

编辑 `.github/workflows/build.yml` 中的 `on` 部分:

```yaml
on:
  push:
    branches: [ main, master, develop ]  # 修改触发分支
  pull_request:
    branches: [ main, master, develop ]  # 修改目标分支
```

### 修改构建产物保留时间

在 `build-cross-compile` 任务中修改:

```yaml
- name: 上传构建产物
  uses: actions/upload-artifact@v4
  with:
    name: asr-rknn-serve-${{ matrix.build_type }}
    path: install/
    retention-days: 30  # 修改保留天数
```

### 添加新的构建类型

在 `build-cross-compile` 任务中修改 `matrix`:

```yaml
strategy:
  matrix:
    build_type: [Release, Debug, RelWithDebInfo]  # 添加新类型
```

## 注意事项

1. **模型文件**: 模型文件（`.rknn`）较大，不在仓库中，需要单独下载并放置到 `resources/model/` 目录

2. **交叉编译工具链**: CI 环境使用 Ubuntu 官方仓库的 `aarch64-linux-gnu` 工具链，与本地环境可能略有差异

3. **构建时间**: 完整的构建流程大约需要 5-10 分钟，取决于网络速度和构建类型

4. **存储空间**: GitHub Actions 对存储空间有限制，旧的构建产物会自动清理

5. **权限**: 创建 Release 需要仓库的写入权限

## 最佳实践

1. **分支策略**: 
   - `develop`: 开发分支
   - `main/master`: 生产分支
   - 通过 Pull Request 进行代码审查

2. **版本管理**: 使用语义化版本号（如 `v1.0.0`、`v1.0.1`）

3. **测试**: 在本地充分测试后再提交到远程仓库

4. **监控**: 定期检查 CI/CD 流程的运行状态，及时发现和解决问题