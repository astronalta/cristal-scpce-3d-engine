/*
    Copyright (C) 1998-2000 by Jorrit Tyberghein

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __BLOCKS_H__
#define __BLOCKS_H__

#include <stdarg.h>
#include "blocdefs.h"
#include "states.h"
#include "cssys/sysdriv.h"
#include "csgeom/math2d.h"
#include "csgeom/math3d.h"
#include "csgeom/matrix3.h"

class csThingTemplate;
class csMaterialHandle;
class csSector;
class csWorld;
class csThing;
class csDynLight;
struct iTextureManager;

class KeyMapping
{
public:
  int key;
  bool shift, alt, ctrl;

public:
  bool Match (int k, bool s, bool a, bool c)
  {
    return key == k && shift == s && alt == a && ctrl == c;
  }
};

enum BlRotType
{
  ROT_NONE = 0,
  ROT_PX,
  ROT_PY,
  ROT_PZ,
  ROT_MX,
  ROT_MY,
  ROT_MZ
};

enum BlShapeType
{
  SHAPE_RANDOM = -1,
  SHAPE_R1,
  SHAPE_R2,
  SHAPE_R3,
  SHAPE_L1,
  SHAPE_L2,
  SHAPE_T1,
  SHAPE_FLAT,
  SHAPE_CUBE,
  SHAPE_L3,
  SHAPE_R4,
  SHAPE_U,
  SHAPE_T2,
  SHAPE_S,
  SHAPE_T1X,
  SHAPE_FLATX,
  SHAPE_FLATXX,
  SHAPE_DEMO_B,
  SHAPE_DEMO_L,
  SHAPE_DEMO_O,
  SHAPE_DEMO_C,
  SHAPE_DEMO_K,
  SHAPE_DEMO_S
};

struct CubeInfo
{
  csThing* thing;
  float dx, dy, dz;
};

class TextEntryMenu
{
private:
  struct TextEntry
  {
    char* txt;
    char* entry;
    void* userdata;
    TextEntry* next;
  };
  TextEntry* entries;
  TextEntry* last;
  int num_entries;
  int selected;
  bool hisel;
  float time_left;

  TextEntry* GetEntry (int num);

public:
  TextEntryMenu ();
  ~TextEntryMenu ();
  void Clear ();
  void Add (const char* txt, const char* entry, void* userdata);
  void ReplaceSelected (const char* txt, const char* entry, void* userdata);
  void HilightSelected (bool sel) { hisel = sel; }
  void Draw (time_t elapsed_time);
  int GetSelected () { return selected; }
  void* GetSelectedData ();
  char* GetSelectedText ();
  char* GetSelectedEntry ();
  int GetNumEntries () { return num_entries; }
  void SetSelected (int sel) { selected = sel; }
  void SelDown ()
  { selected++; if (selected >= num_entries) selected = num_entries - 1; }
  void SelUp () { selected--; if (selected < 0) selected = 0; }




};

class HighScore
{
public:
  char* names[10];
  int scores[10];

public:
  HighScore ();
  ~HighScore ();

  int Get (int i) { return scores[i]; }
  void Set (int i, int value) { scores[i] = value; }
  char* GetName (int i) { return names[i]; }
  void SetName (int i, const char* name);
  bool RegisterScore (const char* name, int score);
  bool CheckScore (int score);
};

class Blocks : public SysSystemDriver
{
private:
  csThingTemplate* cube_tmpl;
  csThingTemplate* pillar_tmpl;
  csThingTemplate* vrast_tmpl;
  csThingTemplate* hrast_tmpl;
  csMaterialHandle* cube_mat;
  csMaterialHandle* cubef1_mat;
  csMaterialHandle* cubef2_mat;
  csMaterialHandle* cubef3_mat;
  csMaterialHandle* cubef4_mat;
  csMaterialHandle* pillar_mat;
  csMaterialHandle* raster_mat;
  csSector* room;
  csSector* demo_room;
  csDynLight* dynlight;
  float dynlight_dx;
  float dynlight_x;
  float dynlight_y;
  float dynlight_z;

  csMatrix3 full_rotate_x;
  csMatrix3 full_rotate_x_reverse;
  csMatrix3 full_rotate_y;
  csMatrix3 full_rotate_y_reverse;
  csMatrix3 full_rotate_z;
  csMatrix3 full_rotate_z_reverse;

  // First dimension is level (0=novice, 1=average, 2=expert).
  // Second dimension is play size (0=3x3, 1=4x4, 2=5x5, 3=6x6).
  HighScore highscores[3][4];
  // If true then we want the player to enter his name for the highscores.
  bool enter_highscore;
  char hs_name[20];
  int hs_pos;

  // For the menu.
  csThing* menus[MAX_MENUS];
  int idx_menus[MAX_MENUS];
  bool leftright_menus[MAX_MENUS];
  csThing* src_menus[MENU_TOTAL];
  csThing* arrow_left;
  csThing* arrow_right;
  int cur_menu;
  int old_cur_menu;
  float menu_todo;
  float menu_hor_todo;
  float menu_hor_old_x_src, menu_hor_old_x_dst;
  float menu_hor_new_x_src, menu_hor_new_x_dst;
  csThing* menu_hor_old_menu;
  int num_menus;	// Current number of active menu entries.

  TextEntryMenu* keyconf_menu;
  bool waiting_for_key;

  csVector3 view_origin;

  // Shift value for rotating a shape.
  // Normally shapes are rotated around (0,0,0).
  // With this vector you can shift this half a block
  // to make sure that rotation always happens around
  // the visual center of the shape and not the logical
  // center.
  csVector3 shift_rotate;

  /*
   * How much distance does the camera still need to move (with
   * 1 being the full distance to move)?
   */
  float cam_move_dist;
  csVector3 cam_move_src, cam_move_dest;
  csVector3 cam_move_up;
  csVector3 destinations[4][5];	// [horizontal][vertical]
  int dest_move_right_dx[4];
  int dest_move_right_dy[4];
  int dest_move_down_dx[4];
  int dest_move_down_dy[4];
  int cur_hor_dest, cur_ver_dest;

  /*
   * How much rotation around the specified axis do we still
   * need to do for the current cube?
   */
  float rot_px_todo;
  float rot_py_todo;
  float rot_pz_todo;
  float rot_mx_todo;
  float rot_my_todo;
  float rot_mz_todo;
  BlRotType queue_rot_todo;

  /*
   * How much distance do we have to move horizontally and in
   * what direction?
   */
  float move_hor_todo;
  int move_hor_dx;
  int move_hor_dy;
  int queue_move_dx_todo;
  int queue_move_dy_todo;
  
  /*
   * The following four flags indicate how the movement keys work.
   * This is to make sure that pressing the right arrow key will always
   * move the block to the right for example.
   */
  int move_right_dx;      
  int move_right_dy;
  int move_down_dx;
  int move_down_dy;

  int num_cubes;
  CubeInfo cube_info[MAX_CUBES];

  /// If true we are paused.
  bool pause;

  // When true clear world and start a new game. Usually false,
  // 'cause we are playing.
  bool initscreen;

  // Type of screen we are using (one of SCREEN_xxx).
  int screen;

  // Difficulty setting.
  int diff_level;

  // Keys...
  KeyMapping key_up;
  KeyMapping key_down;
  KeyMapping key_left;
  KeyMapping key_right;
  KeyMapping key_rotpx;
  KeyMapping key_rotmx;
  KeyMapping key_rotpy;
  KeyMapping key_rotmy;
  KeyMapping key_rotpz;
  KeyMapping key_rotmz;
  KeyMapping key_pause;
  KeyMapping key_drop;
  KeyMapping key_esc;
  KeyMapping key_viewleft;
  KeyMapping key_viewright;
  KeyMapping key_viewup;
  KeyMapping key_viewdown;
  KeyMapping key_zoomin;
  KeyMapping key_zoomout;

public:
  csWorld* world;
  iTextureManager* txtmgr;
  static int white, black, red;

public:
  Blocks ();
  ~Blocks ();

  // Initialization stuff and starting of game/demo.

// Rene.
  bool IsServer;

  void InitTextures ();
  void InitWorld ();
  void InitGameRoom ();
  void InitDemoRoom ();
  void InitMainMenu ();
  void StartNewGame ();
  void StartDemo ();
  void StartKeyConfig ();
  void set_cube_room (csSector* s) { room = s; }
  void InitGame ();
  void CreateMenuEntry (const char* txt, int menu_nr);
  csThing* CreateMenuArrow (bool left);
  void ChangePlaySize (int new_size);

  void ReadConfig ();
  void WriteConfig ();
  void NamedKey (const char* keyname, KeyMapping& map);
  const char* KeyName (const KeyMapping& map);

  void DrawMenu (int menu);
  void DrawMenu (float menu_trans, float menu_hor_trans, int old_menu,
    int new_menu);
  void InitMenu ();
  void AddMenuItem (int menu_nr, bool leftright);
  void ReplaceMenuItem (int idx, int menu_nr);

  // Handling of basic events and frame drawing.
  virtual void NextFrame ();
  virtual bool HandleEvent (csEvent &Event);
  void HandleKey (int key, bool shift, bool alt, bool ctrl);
  void HandleGameKey (int key, bool shift, bool alt, bool ctrl);
  void HandleGameOverKey (int key, bool shift, bool alt, bool ctrl);
  void HandleDemoKey (int key, bool shift, bool alt, bool ctrl);
  void HandleKeyConfigKey (int key, bool shift, bool alt, bool ctrl);
  void HandleHighscoresKey (int key, bool shift, bool alt, bool ctrl);

  // Creating cubes and other geometry.
  csThing* create_cube_thing (float dx, float dy, float dz,
  	csThingTemplate* tmpl);
  csThing* add_cube_thing (csSector* sect, float dx, float dy, float dz,
  	float x, float y, float z, csThingTemplate* tmpl);
  void add_cube (float dx, float dy, float dz, float x, float y, float z,
  	csThingTemplate* tmpl);
  void add_pillar (int x, int y);
  void add_vrast (int x, int y, float dx, float dy, float rot_z);
  void add_hrast (int x, int y, float dx, float dy, float rot_z);

  // All the templates for creating geometry.
  void add_cube_template ();
  void add_pillar_template ();
  void add_vrast_template ();
  void add_hrast_template ();

  // Default textures for geometry.
  void set_cube_material (csMaterialHandle* ct) { cube_mat = ct; }
  void set_cube_f1_material (csMaterialHandle* ct) { cubef1_mat = ct; }
  void set_cube_f2_material (csMaterialHandle* ct) { cubef2_mat = ct; }
  void set_cube_f3_material (csMaterialHandle* ct) { cubef3_mat = ct; }
  void set_cube_f4_material (csMaterialHandle* ct) { cubef4_mat = ct; }
  void set_pillar_material (csMaterialHandle* ct) { pillar_mat = ct; }
  void set_raster_material (csMaterialHandle* ct) { raster_mat = ct; }
  void ChangeThingMaterial (csThing* thing, csMaterialHandle* mat);
  csMaterialHandle* GetMaterialForHeight (int z);

  // Handle all time dependent movement of the game and menu.
  // This function will call some of the Handle... routines below.
  void HandleMovement (time_t elapsed_time);
  // Is called when we are in transition mode instead of HandleMovement()
  // (this happens when a plane is moving down).
  void HandleTransition (time_t elapsed_time);
  // Handle the movement of the camera.
  void HandleCameraMovement ();
  // Handle movement for the startup screen.
  void HandleStartupMovement (time_t elapsed_time);
  // Handle movement for the game screen.
  void HandleGameMovement (time_t elapsed_time);
  // Handle lowering of planes.
  void HandleLoweringPlanes (time_t elapsed_time);

  // Conveniance functions.
  csMatrix3 create_rotate_x (float angle);
  csMatrix3 create_rotate_y (float angle);
  csMatrix3 create_rotate_z (float angle);

  // Start to rotate the current falling block.
  void start_rotation (BlRotType type);
  // Start to move the current falling block horizontally.
  void start_horizontal_move (int dx, int dy);
  // If there is nothing falling down this function causes
  // a new shape to fall down.
  void StartNewShape ();
  // For demo purposes.
  void start_demo_shape (BlShapeType type, float x, float y, float z);

  void move_shape_internal (int dx, int dy, int dz);
  void rotate_shape_internal (const csMatrix3& rot);

  // Return true if free.
  bool check_new_shape_location (int dx, int dy, int dz);
  bool check_new_shape_rotation (const csMatrix3& rot);

  // Shape has stopped.
  void freeze_shape ();

  // Debugging.
  void dump_shape ();

  // Remove all of the planes in the States::filled_planes[] array.
  // Must call States::checkForPlanes first.
  void removePlanesVisual (States* player);

  //------------- Networking stuff.
  
  //The generic connection checker which then calls a client or server checker.
  void CheckConnection();
  
  void ClientCheckConnection();
  void ServerCheckConnection();
  
  
  bool InitNet();
  void Connect ();
  
  void TerminateConnection();
  
  
  // The number of frames since the last network check.
  int since_last_check;
  
  //----------------
  // State Changes.
  States* player1;
  NetworkStates* player1_net;
  
};

#endif // __BLOCKS_H__
