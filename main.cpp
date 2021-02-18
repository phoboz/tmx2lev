#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tile_types.h"
#include "Tmx.h"

enum direction
{
  N, W, S, E, NW, SW, NE, SE, MAX_DIRECTION
};

enum object_type
{
  OBJECT_TYPE_UNKNOWN = 0,
  OBJECT_TYPE_ENEMY,
  OBJECT_TYPE_ITEM,
  OBJECT_TYPE_SAVETUBE
};

enum area_type
{
  AREA_TYPE_UNKNOWN,
  AREA_TYPE_DOOR
};

void write_word(short value, FILE *fp) {
#define SWAP
#ifdef SWAP
  fputc((value & 0xff00) >> 8, fp);
  fputc(value & 0x00ff, fp);
#else
  short to_write = value;
  fwrite(&to_write, sizeof(to_write), 1, fp);
#endif
}

char get_tile_type(const char *str)
{
  char result = TILE_TYPE_NONE;

  if (strcmp(str, "floor") == 0)
    result = TILE_TYPE_FLOOR;
  else if (strcmp(str, "rock") == 0)
    result = TILE_TYPE_ROCK;
  else if (strcmp(str, "metal") == 0)
    result = TILE_TYPE_METAL;
  else if (strcmp(str, "special_1") == 0)
    result = TILE_TYPE_SPECIAL_1;
  else if (strcmp(str, "special_2") == 0)
    result = TILE_TYPE_SPECIAL_2;
  else if (strcmp(str, "overlay") == 0)
    result = TILE_TYPE_OVERLAY;

  return result;
}

int get_max_tiles(const Tmx::Tileset *tileset)
{
  int w  = (tileset->GetImage())->GetWidth();
  int h  = (tileset->GetImage())->GetHeight();

  int tw = tileset->GetTileWidth();
  int th = tileset->GetTileHeight();

  int max_tiles_x = w / tw;
  int max_tiles_y = h / th;

  return max_tiles_x * max_tiles_y;
}

enum object_type get_object_type(const char *str)
{
  enum object_type type;

  if (strcmp(str, "enemy") == 0)
    type = OBJECT_TYPE_ENEMY;
  else if (strcmp(str, "item") == 0)
    type = OBJECT_TYPE_ITEM;
  else if (strcmp(str, "savetube") == 0)
    type = OBJECT_TYPE_SAVETUBE;
  else
    type = OBJECT_TYPE_UNKNOWN;

  return type;
}
enum direction get_direction(const char *str)
{
  enum direction dir;

  if (strcmp(str, "N") == 0)
    dir = N;
  else if (strcmp(str, "W") == 0)
    dir = W;
  else if (strcmp(str, "S") == 0)
    dir = S;
  else if (strcmp(str, "E") == 0)
    dir = E;
  else if (strcmp(str, "NW") == 0)
    dir = NW;
  else if (strcmp(str, "SW") == 0)
    dir = SW;
  else if (strcmp(str, "NE") == 0)
    dir = NE;
  else if (strcmp(str, "SE") == 0)
    dir = SE;
  else
    dir = MAX_DIRECTION;

  return dir;
}

enum area_type get_area_type(const char *str)
{
  enum area_type type;

  if (strcmp(str, "door") == 0)
    type = AREA_TYPE_DOOR;
  else
    type = AREA_TYPE_UNKNOWN;

  return type;
}

int main(int argc, char **argv) {
  if (argc < 3) {
    printf("Usage is: %s <tmxfile> <binfile>\n", argv[0]);
    return 1;
  }

  printf("converting file: %s\n", argv[1]);
  Tmx::Map *map = new Tmx::Map();
printf("create map...\n");
  map->ParseFile(argv[1]);
printf("parsed map...\n");

  if (map->HasError()) {
    printf("error code: %d\n", map->GetErrorCode());
    printf("error text: %s\n", map->GetErrorText().c_str());
    return map->GetErrorCode();
  }

  printf("Version: %1.1f\n", map->GetVersion());

  if (map->GetOrientation() != Tmx::TMX_MO_ORTHOGONAL) {
    printf("error: map orientation must be orthogonal\n");
    return 1;
  }

  FILE *fp = fopen(argv[2], "wb");
  if (fp == NULL) {
    printf("error: unable to create file %s\n", argv[2]);
    return 1;
  }

  const Tmx::Tileset *tileset = map->GetTileset(0);
  if (!tileset) {
    printf("error - no tileset exist\n");
    return 1;
  }

  int num_tiles = get_max_tiles(tileset);
  printf("Number of tiles: %d\n", num_tiles);
  fputc((char) num_tiles, fp);

  std::vector<Tmx::Tile*> tiles = tileset->GetTiles();
  for (int i = 0; i < num_tiles; i++) {

    std::string value;
    char type = TILE_TYPE_NONE;

    for (int j = 0; j < tiles.size(); j++) {

      Tmx::Tile *tile = tiles[j];
      if (i == tile->GetId()) {

        const Tmx::PropertySet prop = tile->GetProperties();
        value = prop.GetLiteralProperty(std::string("type"));

        type = get_tile_type(value.c_str());
        if (type == TILE_TYPE_OVERLAY) {
          std::string value = prop.GetLiteralProperty(std::string("bg_tile"));
          char bg = (char) atoi(value.c_str());
          bg <<= 1;
          type |= bg;
        }

        printf("tile: %d %s(0x%x)\n", i, value.c_str(), type & 0xFF);
        break;
      }
    }

    fputc(type, fp);
  }

  const Tmx::Layer *layer = map->GetLayer(0);
  if (!layer) {
    printf("error: layer zero does not exist\n");
    return 1;
  }

  short w = (short) layer->GetWidth();
  short h = (short) layer->GetHeight();

  printf("Map size: %dx%d\n", w, h);

  write_word(w, fp);
  write_word(h, fp);

#ifdef VERTICAL
  for (int x = 0; x < w; x++) {
    for (int y = 0; y < h; y++) {
      char tile_id = (char) layer->GetTileId(x, y);
      fputc(tile_id, fp);
    }
  }
#else
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      char tile_id = (char) layer->GetTileId(x, y);
      fputc(tile_id, fp);
    }
  }
#endif

  const int num_groups = map->GetNumObjectGroups();
  if (num_groups > 0)
  {
    printf("Found %d object group(s)\n", num_groups);

    int objects_index = -1;
    for (int i = 0; i < num_groups; i++)
    {
      const Tmx::ObjectGroup *group = map->GetObjectGroup(i);
      if (strcmp(group->GetName().c_str(), "objects") == 0)
      {
        objects_index = i;
        break;
      }
    }

    if (objects_index >= 0)
    {
      const Tmx::ObjectGroup *group = map->GetObjectGroup(objects_index);
      const int num_objects = group->GetNumObjects();

      if (num_objects > 0)
      {
        printf("%s has %d object(s)\n", group->GetName().c_str(), num_objects);
        write_word((short) num_objects, fp);

        for (int j = 0; j < num_objects; j++)
        {
          const Tmx::Object *object = group->GetObject(j);

          const std::string type_name = object->GetType();
          enum object_type object_type = get_object_type(type_name.c_str());

          const Tmx::PropertySet prop = object->GetProperties();
          int index = prop.GetNumericProperty(std::string("index"));
          std::string dir_name = prop.GetLiteralProperty(std::string("direction"));
          enum direction dir = get_direction(dir_name.c_str());

          printf("\t%s(%d) - \"%s\" index=%d at: (%d, %d), facing %d(%s)\n", type_name.c_str(), object_type, object->GetName().c_str(), index, object->GetX(), object->GetY(), dir, dir_name.c_str());

          fputc((char) object_type, fp);
          fputc((char) index, fp);
          fputc((char) dir, fp);
          write_word((short) object->GetX(), fp);
          write_word((short) object->GetY(), fp);
        }
      }
    }
    else
    {
      printf("No objects\n");
      write_word((short) 0, fp);
    }


    int areas_index = -1;
    for (int i = 0; i < num_groups; i++)
    {
      const Tmx::ObjectGroup *group = map->GetObjectGroup(i);
      if (strcmp(group->GetName().c_str(), "areas") == 0)
      {
        areas_index = i;
        break;
      }
    }

    if (areas_index >= 0)
    {
      const Tmx::ObjectGroup *group = map->GetObjectGroup(areas_index);
      const int num_objects = group->GetNumObjects();
      if (num_objects > 0)
      {
        printf("%s has %d object(s)\n", group->GetName().c_str(), num_objects);
        write_word((short) num_objects, fp);

        for (int j = 0; j < num_objects; j++)
        {
          const Tmx::Object *object = group->GetObject(j);

          const std::string type_name = object->GetType();
          enum area_type area_type = get_area_type(type_name.c_str());

          const Tmx::PropertySet prop = object->GetProperties();
          int level = prop.GetNumericProperty(std::string("level"));
          int start_x = prop.GetNumericProperty(std::string("start_x"));
          int start_y = prop.GetNumericProperty(std::string("start_y"));
          std::string dir_name = prop.GetLiteralProperty(std::string("direction"));
          enum direction dir = get_direction(dir_name.c_str());

          printf("\t%s(%d) - \"%s\" at: (%d, %d) size(%d x %d), level %d, start (%d, %d), facing %d(%s)\n", type_name.c_str(), area_type, object->GetName().c_str(), object->GetX(), object->GetY(), object->GetWidth(), object->GetHeight(), level, start_x, start_y, dir, dir_name.c_str());

          fputc((char) area_type, fp);
          write_word((short) level, fp);
          write_word((short) start_x, fp);
          write_word((short) start_y, fp);
          fputc((char) dir, fp);
          write_word((short) object->GetX(), fp);
          write_word((short) object->GetY(), fp);
          write_word((short) object->GetWidth(), fp);
          write_word((short) object->GetHeight(), fp);
        }
      }
    }
    else
    {
      printf("No areas\n");
      write_word((short) 0, fp);
    }
  }
  else
  {
    printf("No object group(s) defined\n");
    write_word((short) 0, fp);
    write_word((short) 0, fp);
  }

  return 0;
}

