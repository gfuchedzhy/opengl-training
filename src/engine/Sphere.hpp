/*
 * Copyright 2015 Grygoriy Fuchedzhy <grygoriy.fuchedzhy@gmail.com>
 */

#ifndef ENGINE_SPHERE_HPP
#define ENGINE_SPHERE_HPP

#include "Model.hpp"

/// @brief renderable sphere that has color and model
class CSphere : public IRenderableModel
{
      /// @brief color
      glm::vec3 mColor;

   public:
      /// @brief constructor
      CSphere(const glm::vec3& color);

      /// @brief draw
      void draw(const SContext& context) const override;
};

#endif