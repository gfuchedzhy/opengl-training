/*
 * Copyright 2015 Grygoriy Fuchedzhy <grygoriy.fuchedzhy@gmail.com>
 */

#include "Aircraft.hpp"
#include "Box.hpp"
#include "Rect.hpp"
#include "Context.hpp"
#include "Programs.hpp"
#include <algorithm>
#include <glm/gtx/transform.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

CAircraft::CAircraft()
{
   Assimp::Importer importer;
   const aiScene* scene = importer.ReadFile("res/Su-35_SuperFlanker/Su-35_SuperFlanker.obj",
                                            aiProcess_CalcTangentSpace       |
                                            aiProcess_JoinIdenticalVertices  |
                                            aiProcess_SortByPType);
   if (!scene)
      throw std::runtime_error{"couldn't load aircraft model"};

   std::vector<std::shared_ptr<SMaterial>> materials;
   for (size_t m = 0; m < scene->mNumMaterials; ++m)
   {
      const aiMaterial* material = scene->mMaterials[m];
      assert(material);
      materials.push_back(std::make_shared<SMaterial>(*material, "res/Su-35_SuperFlanker/"));
   }

   for (size_t m = 0; m < scene->mNumMeshes; ++m)
   {
      const aiMesh* const mesh = scene->mMeshes[m];
      assert(mesh);
      mMeshes.emplace_back(std::make_unique<CMesh>(*mesh, materials[mesh->mMaterialIndex]));
   }

   using namespace std;
   using namespace glm;

   auto make_box = [] (vec3 color, vec3 size) {
      auto p = make_shared<CBox>(color);
      p->scale(size);
      return p;
   };

   auto add_hb = [this] (shared_ptr<IRenderableModel> r) {
      CHealthBar b;
      b.size({3, 0.5});
      mHealthBars.push_back(make_pair(b, r.get()));
   };

   // cubed geometry representing airplain, kind of :)
   const vec3 bodySize(3.f, 22.f, 3.f);
   const int bodyPartNum = 4;
   const vec3 bodyPartSize(3.f, bodySize.y/bodyPartNum, 3.f);
   const vec3 bodyColor(1.f, 1.f, 0.f);

   auto body = make_shared<CComplexRenderable>();
   for (int i = 0; i < bodyPartNum; ++i)
   {
      auto p = make_box(bodyColor, bodyPartSize);
      add_hb(p);
      p->pos(vec3(0.f, -(i + 0.5f)*bodyPartSize.y, 0.f));
      body->append({p});
   }

   const vec3 tailColor(1.f, 1.f, 1.f);
   auto hTail = make_box(tailColor, {9.f, 5.f, 0.2f});
   auto vTail = make_box(tailColor, {0.2f, 5.f, 5.f});
   hTail->pos(vec3(0.f, 2.5f, 0.f));
   vTail->pos(vec3(0.f, 2.5f, 2.5f));
   auto tail = make_shared<CComplexRenderable>(tIniList{hTail, vTail});
   tail->pos(vec3(0.f, -bodySize.y, 0.f));

   const vec3 wingColor(0.f, 1.f, 0.f);
   const vec3 wingSize(12.f, 4.f, 0.2f);
   auto lWing = make_box(wingColor, wingSize);
   auto rWing = make_box(wingColor, wingSize);
   add_hb(lWing);
   add_hb(rWing);
   lWing->yaw( 20.f);
   rWing->yaw(-20.f);
   lWing->pos(vec3(-wingSize.x/2.f, 0.f, 0.f));
   rWing->pos(vec3( wingSize.x/2.f, 0.f, 0.f));
   auto wings = make_shared<CComplexRenderable>(tIniList{lWing, rWing});
   wings->pos(vec3(0.f, -bodySize.y/3.f, 0.f));

   const vec3 propColor(0.f, 1.f, 1.f);
   const vec3 propSize(1.f, 0.2f, 12.f);
   const int propNum = 3;
   auto prop = make_shared<CComplexRenderable>();
   for (int i = 0u; i < propNum; ++i)
   {
      auto blade = make_box(propColor, propSize);
      blade->roll(i*180.f/propNum);
      prop->append({blade});
   }
   auto shaft = make_box(propColor, {0.3f, 0.8f, 0.3f});
   shaft->pos(vec3(0.f, -0.4f, 0.f));
   prop->append({shaft});
   prop->pos(vec3(0.f, 0.4f, 0.f));
   auto wholeBody = make_shared<CComplexRenderable>(tIniList{body, tail, wings, prop});
   wholeBody->pos(vec3(0.f, bodySize.y*0.2f, -bodySize.z/2));
   auto cabin = make_box({1.f, 0.f, 0.f}, {1.5f, 3.f, 2.f});
   append(tIniList{wholeBody, cabin});
   mProp = prop.get();
}

void CAircraft::update(float timeDelta)
{
   if (0 != mSpeed && pos().z <= 0)
   {
      mSpeed = 0;
      for (auto&& h : mHealthBars)
         h.first.value(0);
   }

   pos(pos() + forward()*mSpeed*timeDelta);

   const float angularSpeed = 7*mSpeed; // @todo should be around 150x
   float angle = mProp->roll() + timeDelta*angularSpeed;
   angle = angle - (int(angle)/360)*360;
   mProp->roll(angle);
}

void CAircraft::draw(const SContext& context) const
{
   CComplexRenderable::draw(context);
   auto p = gRenderer.getAndSelect<cts("texturedPolygon")>();
   p->set<cts("uModel")>(model());

   for (const auto& m: mMeshes)
   {
      m->draw(context);
   }

   if (context.mDrawHealthBars)
   {
      auto p = gRenderer.getAndSelect<cts("healthbar")>();
      for (auto&& h : mHealthBars)
      {
         // @todo this is a hack, should rethink model of complex objects
         assert(h.second);
         auto m = h.second->model();
         p->set<cts("uExternalPos")>(glm::vec3(m[3][0], m[3][1], m[3][2]));
         h.first.draw(context);
         p->set<cts("uExternalPos")>({0, 0, 0});
      }
   }
}

void CAircraft::randomDamage()
{
   std::uniform_real_distribution<> d(0, 1);
   for (auto&& h : mHealthBars)
      h.first.value(d(random_gen));
}

void CAircraft::repair()
{
   auto m = std::min_element(begin(mHealthBars), end(mHealthBars),
                             [] (const tHealthBar& a, const tHealthBar& b)
                             {
                                return a.first.value() < b.first.value();
                             });
   assert(m != end(mHealthBars));
   m->first.value(std::min(m->first.value() + 0.5f, 1.f));
}
