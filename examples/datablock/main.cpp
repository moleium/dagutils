#include <datablock/datablock.h>
#include <debug/dag_assert.h>
#include <math/dag_Point3.h>
#include <memory/dag_mem.h>
#include <print>
#include <stdio.h>

void (*dgs_fatal_report)(const char *, const char *) = nullptr;

int main() {
  dagor_force_init_memmgr();

  DataBlock settings;
  settings.addStr("player_name", "test_user");
  settings.addInt("score", 1000);
  settings.addBool("sound_enabled", true);

  DataBlock *video_settings = settings.addNewBlock("video");
  video_settings->addInt("width", 1920);
  video_settings->addInt("height", 1080);
  video_settings->setStr("quality", "high");

  const char *temp_name = "settings.blk";
  if (settings.saveToTextFile(temp_name)) {
    FILE *file = fopen(temp_name, "r");
    if (file) {
      char line[256];
      while (fgets(line, sizeof(line), file)) {
        std::print("{}", line);
      }
      fclose(file);
      remove(temp_name);
    }
  }

  return 0;
}