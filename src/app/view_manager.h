#pragma once

#include <functional>

namespace homedeck {

enum class SystemView {
  Almanac,
  Calendar,
  Countdown,
  Weather,
};

struct ViewManagerDeps {
  std::function<void()> renderAlmanac;
  std::function<void()> renderCalendar;
  std::function<void()> renderCountdown;
  std::function<void()> renderWeather;
};

class ViewManager {
 public:
  explicit ViewManager(ViewManagerDeps deps);
  void begin(SystemView initialView = SystemView::Almanac);
  void switchToNextView();
  SystemView currentView() const;

 private:
  void switchTo(SystemView view);

  ViewManagerDeps deps_;
  SystemView currentView_ = SystemView::Almanac;
};

}  // namespace homedeck
