

## Task 7: 最终提交

- [ ] **Step 1: 检查 git status**

  Run: `git status`
  Expected: 工作区干净，所有变更已提交

- [ ] **Step 2: 查看提交历史**

  Run: `git log --oneline -7`
  Expected: 看到以下提交（或类似）：
  1. `feat: BootController 新增 preSleepRender 回调`
  2. `test: BootController preSleepRender 测试`
  3. `feat: CalendarData 支持底部中间文字`
  4. `test: HomeRenderer 底部文字通用化测试`
  5. `feat: app_runtime 集成 preSleepRender + 简化 deep sleep`
  6. `chore: 全量测试通过验证`

---

## Self-Review Checklist

### Spec Coverage

| 设计文档需求 | 对应 Task |
|-------------|----------|
| `BootControllerDeps` 新增 `preSleepRender` | Task 1 |
| `updateHomeSleep()` 重置 offset + 调用 `preSleepRender` | Task 1 |
| `enterHomeDeepSleep()` 不再渲染 | Task 5 |
| `CalendarData` 新增 `bottomCenterMessage` | Task 3 |
| `drawEnvironmentReadings()` 通用化 | Task 3 |
| `renderCalendar()` 传递底部文字 | Task 3 |
| `preSleepRender` lambda 实现（HH:MM + 视图分支） | Task 5 |
| 删除 `renderHomeWithDeepSleepMessage()` | Task 5 |
| BootController 测试（调用顺序 + offset 重置 + 视图传递） | Task 2 |
| HomeRenderer 测试（通用化 + 日历底部文字） | Task 4 |

### Placeholder Scan

- [x] 无 TBD / TODO / "implement later"
- [x] 无 "add appropriate error handling" 等模糊描述
- [x] 每步都有完整代码或精确命令
- [x] 测试代码包含断言和预期输出

### Type Consistency

- [x] `preSleepRender` 签名：`std::function<void(SystemView)>` — 在 `boot_controller.h`、`boot_controller.cpp`、Fixture mock、lambda 中一致
- [x] `bottomCenterMessage` 字段名在 `HomeCalendarData` 和 `CalendarData` 中一致
- [x] `SystemView` 枚举值 `Almanac` / `Calendar` 在各处使用正确

---

## Execution Handoff

**Plan complete and saved to `docs/superpowers/plans/2026-05-28-deep-sleep-display-optimization.md`.**

**Two execution options:**

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration

**2. Inline Execution** — Execute tasks in this session using executing-plans, batch execution with checkpoints

**Which approach?**