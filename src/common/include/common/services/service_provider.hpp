#pragma once

#include "../threading/task_service.hpp"

namespace aln
{
class ServiceProvider
{
  private:
    TaskService m_taskService;

  public:
    TaskService* GetTaskService() { return &m_taskService; }
};
} // namespace aln