/*
   Copyright 2013 Barobo, Inc.

   This file is part of BaroboLink.

   BaroboLink is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   BaroboLink is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with BaroboLink.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "RecordMobot.h"
#ifdef _MSYS
#include <windows.h>
#endif

#define RAD2DEG(x) ((x)*180.0/M_PI)

recordMobot_t* RecordMobot_new()
{
  recordMobot_t* mobot;
  mobot = (recordMobot_t*)malloc(sizeof(recordMobot_t));
  memset(mobot, 0, sizeof(recordMobot_t));
  mobot->motions = (struct motion_s**)malloc(sizeof(struct motion_s*) * 100); 
  memset(mobot->motions, 0, sizeof(struct motion_s*) * 100);
  mobot->numMotionsAllocated = 100;
  return mobot;
}

void RecordMobot_init(recordMobot_t* mobot, const char *name)
{
  Mobot_init((mobot_t*)mobot);
  mobot->firmwareVersion = 0x0FFFFFFF;
  mobot->numMotions = 0;
  mobot->bound = false;
  mobot->connectStatus = RMOBOT_NOT_CONNECTED;
  strcpy(mobot->name, name);
  mobot->dirty = 0;
}

void RecordMobot_destroy(recordMobot_t* mobot)
{
  free(mobot->motions);
}

int RecordMobot_connectWithAddress(recordMobot_t* mobot, const char address[], int channel)
{
  int rc;
  int numPoses;
  double a1, a2, a3, a4;
  int i;
  struct motion_s* motion;
  char *posename;
  strcpy(mobot->address, address);
  mobot->connectStatus = RMOBOT_CONNECTING;
  if(rc = Mobot_connectWithAddress((mobot_t*)mobot, address, channel)) {
    mobot->connectStatus = RMOBOT_NOT_CONNECTED;
    return rc;
  }
  mobot->firmwareVersion = Mobot_getVersion((mobot_t*)mobot);
    mobot->connectStatus = RMOBOT_CONNECTED;
  if(strlen(address)==4) {
    /* If it was a linkbot, get all of its poses */
    rc = Mobot_getNumPoses((mobot_t*)mobot, &numPoses);
    for(i = 0; i < numPoses; i++) {
      rc = Mobot_getPoseData((mobot_t*)mobot, i, &a1, &a2, &a3, &a4);
      motion = (struct motion_s*)malloc(sizeof(struct motion_s));
      motion->motionType = MOTION_POS;
      motion->data.pos[0] = a1;
      motion->data.pos[1] = a2;
      motion->data.pos[2] = a3;
      motion->data.pos[3] = 0;
      posename = (char*)malloc(32);
      sprintf(posename, "Pose %d", mobot->numMotions + 1);
      motion->name = posename;
      mobot->motions[mobot->numMotions] = motion;
      mobot->numMotions++;
    }
  }
  mobot->dirty = 1;
  return 0;
}

int RecordMobot_connectWithZigbeeAddress(recordMobot_t* mobot, uint16_t addr)
{
  int rc;
  double a1, a2, a3, a4;
  int numPoses;
  int i;
  struct motion_s* motion;
  char *posename;
  mobot->connectStatus = RMOBOT_CONNECTING;
  if(rc = Mobot_connectWithZigbeeAddress((mobot_t*)mobot, addr)) {
    mobot->connectStatus = RMOBOT_NOT_CONNECTED;
    return rc;
  }
  mobot->firmwareVersion = Mobot_getVersion((mobot_t*)mobot);
    mobot->connectStatus = RMOBOT_CONNECTED;
  mobot->dirty = 1;
  strcpy(mobot->address, mobot->mobot.serialID);
  /* If it was a linkbot, get all of its poses */
  rc = Mobot_getNumPoses((mobot_t*)mobot, &numPoses);
  for(i = 0; i < numPoses; i++) {
    rc = Mobot_getPoseData((mobot_t*)mobot, i, &a1, &a2, &a3, &a4);
    motion = (struct motion_s*)malloc(sizeof(struct motion_s));
    motion->motionType = MOTION_POS;
    motion->data.pos[0] = a1;
    motion->data.pos[1] = a2;
    motion->data.pos[2] = a3;
    motion->data.pos[3] = 0;
    posename = (char*)malloc(32);
    sprintf(posename, "Pose %d", mobot->numMotions + 1);
    motion->name = posename;
    mobot->motions[mobot->numMotions] = motion;
    mobot->numMotions++;
  }
  return 0;
}

const char* RecordMobot_getAddress(recordMobot_t* mobot)
{
  return mobot->address;
}

int RecordMobot_record(recordMobot_t* mobot)
{
	/* Get the robots positions */
  char *poseName;
  int i;
	double angles[4];
  char text[32];
	Mobot_getJointAngles((mobot_t*)mobot, &angles[0], &angles[1], &angles[2], &angles[3]);
	struct motion_s* motion;
	motion = (struct motion_s*)malloc(sizeof(struct motion_s));
	motion->motionType = MOTION_POS;
  for(i = 0; i < 4; i++) {
    /* Save angles */
    motion->data.pos[i] = angles[i];
  }

  poseName = (char*)malloc(32);
  sprintf(poseName, "Pose %d", mobot->numMotions + 1);
	motion->name = poseName;
	mobot->motions[mobot->numMotions] = motion;
	mobot->numMotions++;
	return 0;
}

int RecordMobot_addDelay(recordMobot_t* mobot, double seconds)
{
	struct motion_s* motion;
	motion = (struct motion_s*)malloc(sizeof(struct motion_s));
	motion->motionType = MOTION_SLEEP;
	motion->data.sleepDuration = seconds;
	motion->name = strdup("Delay");
	mobot->motions[mobot->numMotions] = motion;
	mobot->numMotions++;
	return 0;
}

int RecordMobot_isMoving(recordMobot_t* rmobot)
{
  mobot_t *mobot = (mobot_t*)rmobot;
  mobotJointState_t state;
  int i;
  for(i = 1; i <= 4; i++) {
    Mobot_getJointState(mobot, (robotJointId_t)i, &state);
    if(state == ROBOT_FORWARD || state == ROBOT_BACKWARD) {
      return 1;
    }
  }
  return 0;
}

int RecordMobot_play(recordMobot_t* mobot, int index)
{
	if (index < 0 || index >= mobot->numMotions) {
		return -1;
	}
	if(mobot->motions[index]->motionType == MOTION_POS) {
    /* Make sure the motion angles are up to date with the gtkentries */
		return Mobot_moveToNB( (mobot_t*)mobot,
			mobot->motions[index]->data.pos[0],
			mobot->motions[index]->data.pos[1],
			mobot->motions[index]->data.pos[2],
			mobot->motions[index]->data.pos[3]
		);
	} else if (mobot->motions[index]->motionType == MOTION_SLEEP) {
#ifdef _MSYS
    Sleep(mobot->motions[index]->data.sleepDuration * 1000);
#else
		usleep(mobot->motions[index]->data.sleepDuration * 1000000);
#endif
		return 0;
	}
}

int RecordMobot_getMotionType(recordMobot_t* mobot, int index)
{
	if (index < 0 || index >= mobot->numMotions) {
		return -1;
	}
	return mobot->motions[index]->motionType;
}

int RecordMobot_getChMotionString(recordMobot_t* mobot, int index, char* buf)
{
  switch(mobot->motions[index]->motionType) {
    case MOTION_POS:
      switch(mobot->mobot.formFactor) {
        case MOBOTFORM_I:
          sprintf(buf, "%s.moveToNB(%.2lf, NaN, %.2lf);",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[2]));
          break;
        case MOBOTFORM_L:
          sprintf(buf, "%s.moveToNB(%.2lf, %.2lf, NaN);",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[1]));
          break;
        case MOBOTFORM_ORIGINAL:
        default:
          sprintf(buf, "%s.moveToNB(%.2lf, %.2lf, %.2lf, %.2lf);",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[1]),
              RAD2DEG(mobot->motions[index]->data.pos[2]),
              RAD2DEG(mobot->motions[index]->data.pos[3]) );
          break;
      }
      break;
    case MOTION_SLEEP:
      sprintf(buf, "delay(%.2lf);", (mobot->motions[index]->data.sleepDuration));
      break;
  }
	return 0;
}

int RecordMobot_getChMotionStringB(recordMobot_t* mobot, int index, char* buf)
{
  switch(mobot->motions[index]->motionType) {
    case MOTION_POS:
      switch(mobot->mobot.formFactor) {
        case MOBOTFORM_I:
          sprintf(buf, "%s.moveTo(%.2lf, NaN, %.2lf);",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[2]));
          break;
        case MOBOTFORM_L:
          sprintf(buf, "%s.moveTo(%.2lf, %.2lf, NaN);",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[1]));
          break;
        case MOBOTFORM_ORIGINAL:
        default:
          sprintf(buf, "%s.moveTo(%.2lf, %.2lf, %.2lf, %.2lf);",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[1]),
              RAD2DEG(mobot->motions[index]->data.pos[2]),
              RAD2DEG(mobot->motions[index]->data.pos[3]) );
          break;
      }
      break;
    case MOTION_SLEEP:
      sprintf(buf, "delay(%.2lf);", (mobot->motions[index]->data.sleepDuration));
      break;
  }
	return 0;
}

int RecordMobot_getPythonMotionString(recordMobot_t* mobot, int index, char* buf)
{
  switch(mobot->motions[index]->motionType) {
    case MOTION_POS:
      switch(mobot->mobot.formFactor) {
        case MOBOTFORM_I:
          sprintf(buf, "%s.moveToNB(%.2lf, 0, %.2lf)",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[2]));
          break;
        case MOBOTFORM_L:
          sprintf(buf, "%s.moveToNB(%.2lf, %.2lf, 0)",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[1]));
          break;
        case MOBOTFORM_ORIGINAL:
        default:
          sprintf(buf, "%s.moveToNB(%.2lf, %.2lf, %.2lf, %.2lf)",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[1]),
              RAD2DEG(mobot->motions[index]->data.pos[2]),
              RAD2DEG(mobot->motions[index]->data.pos[3]) );
          break;
      }
      break;
    case MOTION_SLEEP:
      sprintf(buf, "time.sleep(%.2lf)", (mobot->motions[index]->data.sleepDuration));
      break;
  }
	return 0;
}

int RecordMobot_getPythonMotionStringB(recordMobot_t* mobot, int index, char* buf)
{
  switch(mobot->motions[index]->motionType) {
    case MOTION_POS:
      switch(mobot->mobot.formFactor) {
        case MOBOTFORM_I:
          sprintf(buf, "%s.moveTo(%.2lf, 0, %.2lf)",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[2]));
          break;
        case MOBOTFORM_L:
          sprintf(buf, "%s.moveTo(%.2lf, %.2lf, 0)",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[1]));
          break;
        case MOBOTFORM_ORIGINAL:
        default:
          sprintf(buf, "%s.moveTo(%.2lf, %.2lf, %.2lf, %.2lf)",
              mobot->name,
              RAD2DEG(mobot->motions[index]->data.pos[0]),
              RAD2DEG(mobot->motions[index]->data.pos[1]),
              RAD2DEG(mobot->motions[index]->data.pos[2]),
              RAD2DEG(mobot->motions[index]->data.pos[3]) );
          break;
      }
      break;
    case MOTION_SLEEP:
      sprintf(buf, "time.sleep(%.2lf)", (mobot->motions[index]->data.sleepDuration));
      break;
  }
	return 0;
}

GtkWidget* create_angle_entry(double angle)
{
  char text[64];
  GtkWidget* entry = gtk_entry_new();
  sprintf(text, "%.2lf", angle);
  gtk_entry_set_text(GTK_ENTRY(entry), text);
  gtk_entry_set_width_chars(GTK_ENTRY(entry), 7);
  return entry;
}

int RecordMobot_getPythonInteractiveMotionStringB(GtkWidget* vbox, recordMobot_t* mobot, int index)
{
  /* Create a new HBox to pack into the vbox */
  char buf[128];
  /* Create GTK Widgets */
  int i;
  for(i = 0; i < 4; i++) {
    mobot->motions[index]->angleEntries[i] = NULL;
  }
  GtkWidget *hbox = gtk_hbox_new(false, 0);
  switch(mobot->motions[index]->motionType) {
    case MOTION_POS:
      switch(mobot->mobot.formFactor) {
        case MOBOTFORM_I:
          sprintf(buf, "%s.moveTo(", mobot->name);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(buf), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[0] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[0]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[0], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", 0, "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[2] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[2]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[2], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(")"), TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
          gtk_widget_show_all(hbox);
          break;
        case MOBOTFORM_L:
          sprintf(buf, "%s.moveTo(", mobot->name);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(buf), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[0] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[0]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[0], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[1] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[1]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[1], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", 0)"), TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
          gtk_widget_show_all(hbox);
          break;
        case MOBOTFORM_ORIGINAL:
        default:
          sprintf(buf, "%s.moveTo(", mobot->name);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(buf), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[0] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[0]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[0], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[1] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[1]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[1], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[2] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[2]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[2], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[3] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[3]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[3], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(")"), TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
          gtk_widget_show_all(hbox);
          break;
      }
      break;
    case MOTION_SLEEP:
      sprintf(buf, "time.sleep(%.2lf)", (mobot->motions[index]->data.sleepDuration));
      break;
  }
	return 0;
}

int RecordMobot_getPythonInteractiveMotionString(GtkWidget* vbox, recordMobot_t* mobot, int index)
{
  /* Create a new HBox to pack into the vbox */
  char buf[128];
  GtkWidget *hbox = gtk_hbox_new(false, 0);
  switch(mobot->motions[index]->motionType) {
    case MOTION_POS:
      switch(mobot->mobot.formFactor) {
        case MOBOTFORM_I:
          sprintf(buf, "%s.moveToNB(", mobot->name);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(buf), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[0] = 
            create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[0]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[0], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", 0, "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[2] = 
            create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[2]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[2], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(")"), TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
          gtk_widget_show_all(hbox);
          break;
        case MOBOTFORM_L:
          sprintf(buf, "%s.moveToNB(", mobot->name);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(buf), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[0] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[0]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[0], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[1] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[1]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[1], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", 0)"), TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
          gtk_widget_show_all(hbox);
        case MOBOTFORM_ORIGINAL:
        default:
          sprintf(buf, "%s.moveToNB(", mobot->name);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(buf), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[0] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[0]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[0], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[1] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[1]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[1], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[2] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[2]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[2], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(", "), TRUE, TRUE, 0);
          mobot->motions[index]->angleEntries[3] = create_angle_entry(RAD2DEG(mobot->motions[index]->data.pos[3]));
          gtk_box_pack_start(GTK_BOX(hbox), mobot->motions[index]->angleEntries[3], TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new(")"), TRUE, TRUE, 0);
          gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
          gtk_widget_show_all(hbox);
          break;
      }
      break;
    case MOTION_SLEEP:
      sprintf(buf, "time.sleep(%.2lf)", (mobot->motions[index]->data.sleepDuration));
      break;
  }
	return 0;
}

const char* RecordMobot_getMotionName(recordMobot_t* mobot, int index)
{
	if(index < 0 || index >= mobot->numMotions) {
		return NULL;
	}
	return mobot->motions[index]->name;
}

int RecordMobot_setMotionName(recordMobot_t* mobot, int index, const char* name)
{
	if(index < 0 || index >= mobot->numMotions) {
		return -1;
	}
	free (mobot->motions[index]->name);
	mobot->motions[index]->name = strdup(name);
	return 0;
}

int RecordMobot_removeMotion(recordMobot_t* mobot, int index, bool releaseData)
{
	if(index < 0 || index >= mobot->numMotions) {
		return -1;
	}
	/* Free the motion */
  if(releaseData) {
    free(mobot->motions[index]);
  }
	/* Shift everything lower than the motion up by one */
	int i;
	for(i = index+1; i < mobot->numMotions; i++) {
		mobot->motions[i-1] = mobot->motions[i];
	}
	mobot->numMotions--;
	return 0;
}

int RecordMobot_clearAllMotions(recordMobot_t* mobot)
{
  int i;
  for(i = 0; i < mobot->numMotions; i++) {
    free(mobot->motions[i]);
  }
  mobot->numMotions = 0;
  return 0;
}

int RecordMobot_moveMotion(recordMobot_t* mobot, int fromindex, int toindex)
{
  if(fromindex < 0 || fromindex >= mobot->numMotions) {
    return -1;
  }
  if(toindex < 0 || toindex >= mobot->numMotions) {
    return -1;
  }
  if(fromindex == toindex) {
    return 0;
  }
  /* Save the motion at fromindex */
  struct motion_s* motion = mobot->motions[fromindex];
  /* Now, remove mobot->motions[fromindex] */
  RecordMobot_removeMotion(mobot, fromindex, false);
  //if(toindex > fromindex) {toindex--;}
  /* Make a space for the new motion at 'toindex' */
  int i;
  for(i = mobot->numMotions - 1; i >= toindex; i--) {
    mobot->motions[i+1] = mobot->motions[i];
  }
  mobot->motions[toindex] = motion;
  mobot->numMotions++;
  
  return 0;
}

int RecordMobot_swapMotion(recordMobot_t* mobot, int index1, int index2)
{
  if(index1 == index2) {
    return 0;
  }
  if(index1 < 0 || index1 > mobot->numMotions) {
    return -1;
  }
  if(index2 < 0 || index2 > mobot->numMotions) {
    return -1;
  }
  struct motion_s* tmp;
  tmp = mobot->motions[index1];
  mobot->motions[index1] = mobot->motions[index2];
  mobot->motions[index2] = tmp;
  return 0;
}

void RecordMobot_setName(recordMobot_t* mobot, const char* name)
{
  strncpy(mobot->name, name, 78);
  mobot->name[79] = '\0';
}

recordMobotConnectStatus_t RecordMobot_connectStatus(recordMobot_t* mobot)
{
  return mobot->connectStatus;
}

void motion_update(struct motion_s* motion)
{
  if(motion->motionType == MOTION_SLEEP) {
    return;
  }
  int i;
  const char* text;
  for(i = 0; i < 4; i++) {
    if(motion->angleEntries[i] == NULL) {
      continue;
    }
    text = gtk_entry_get_text(GTK_ENTRY(motion->angleEntries[i]));
    sscanf(text, "%lf", &motion->data.pos[i]);
  }
}
