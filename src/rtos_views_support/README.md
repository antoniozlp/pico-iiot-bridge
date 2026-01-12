# RTOS Views Support

This module provides runtime statistics support for FreeRTOS, enabling VS Code's RTOS Views to display detailed task information including CPU usage percentages.

## Features

When enabled, RTOS Views will show:
- ✅ **Stack Size** - Total stack allocated for each task
- ✅ **Stack Used** - Current stack usage
- ✅ **Stack Peak** - Maximum stack usage (high water mark)
- ✅ **Runtime %** - CPU time percentage per task

## Configuration

### Enable RTOS Views Support (Default)

RTOS Views support is **enabled by default** at the top of `CMakeLists.txt` (before FreeRTOS configuration):

```cmake
# In CMakeLists.txt, in the FreeRTOS section:
add_definitions(-DRTOS_VIEWS_SUPPORT)
```

### Disable for Production Builds

To disable runtime statistics for better performance in production, comment out the definition in `CMakeLists.txt` (around line 43, in the FreeRTOS section):

```cmake
# add_definitions(-DRTOS_VIEWS_SUPPORT)
```

**Important**: The definition must be placed BEFORE `add_subdirectory(lib/FreeRTOS-config)` to take effect.