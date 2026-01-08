# Mouse Enhancer

Windows鼠标增强软件，提供快捷键悬浮按钮功能。

## 功能特性

1. **鼠标静止检测**：当鼠标光标静止1.5秒后，自动在光标左右两侧显示圆角矩形按钮
2. **自定义键位**：左右两个按钮可以分别设置为不同的键盘按键（默认：左键=Enter，右键=Esc）
3. **智能隐藏**：鼠标移动后，悬浮按钮自动消失
4. **系统托盘**：程序运行时在系统托盘显示图标
5. **右键菜单**：右键点击托盘图标可以打开设置或退出程序

## 编译方法

使用GCC编译器编译：

```bash
gcc main.c -o mouse_enhancer.exe -mwindows -luser32 -lgdi32 -lcomctl32
```

或者直接运行编译脚本：

```bash
build.bat
```

## 使用方法

1. 运行编译后的 `mouse_enhancer.exe`
2. 程序会在系统托盘显示图标
3. 将鼠标光标静止1.5秒，会看到两个蓝色圆角按钮出现在光标附近
4. 点击左侧按钮发送Enter键，点击右侧按钮发送Esc键
5. 右键点击系统托盘图标可以：
   - 打开设置对话框修改键位
   - 退出程序

## 配置文件

程序会在当前目录创建 `config.ini` 文件保存配置：

```
leftKey=13
rightKey=27
```

键位代码说明：
- 13: Enter
- 27: Escape
- 32: Space
- 9: Tab
- 8: Backspace
- 46: Delete
- 45: Insert
- 36: Home
- 35: End
- 33: Page Up
- 34: Page Down

## 技术实现

- 使用Windows低级鼠标钩子（WH_MOUSE_LL）检测鼠标移动
- 使用SetTimer定时器检测鼠标静止状态
- 使用GDI绘制圆角矩形按钮
- 使用系统托盘API（Shell_NotifyIcon）管理托盘图标
- 使用INI文件保存用户配置

## 系统要求

- Windows操作系统
- GCC编译器（MinGW）
- Windows API支持
