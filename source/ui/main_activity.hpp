#pragma once

#include <borealis.hpp>

namespace ui
{

// Root activity: a sidebar TabFrame with Buscar / Novidades / Biblioteca / Configurações.
class MainActivity : public brls::Activity
{
  public:
    brls::View* createContentView() override;
};

} // namespace ui
