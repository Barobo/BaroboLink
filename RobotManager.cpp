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

#include <stdio.h>
#include <stdlib.h>
#include "BaroboLink.h"
#include "RobotManager.h"
#include "thread_macros.h"

CRobotManager::CRobotManager()
{
  int i;
  for(i = 0; i < MAX_CONNECTED; i++) {
    _mobots[i] = NULL;
  }
  _isPlaying = false;
}

CRobotManager::~CRobotManager()
{
}

bool CRobotManager::isConnected(int index) 
{
  if((index >= numEntries()) || index < 0) {
    return false;
  }
  if(_mobots[index] == NULL) {
    return false;
  }
  return Mobot_isConnected((mobot_t*)_mobots[index]);
}

bool CRobotManager::isConnectedZigbee(uint16_t addr)
{
  int i;
  for(i = 0; i < MAX_CONNECTED; i++) {
    if(_mobots[i]) {
      if(
          (_mobots[i]->mobot.zigbeeAddr == addr) &&
          (_mobots[i]->mobot.connected)
        )
      {
        return true;
      }
    }
  }
  return false;
}

int CRobotManager::addEntry(const char* entry)
{
  int rc;
  if(rc = ConfigFile::addEntry(entry)) {
    return rc;
  }

  /* Adjust the array of mobots */
  for(int i = (numEntries()-1); i >= 0; i--) {
    _mobots[i+1] = _mobots[i];
  }
  _mobots[0] = NULL;
}

int CRobotManager::addMobot(recordMobot_t* mobot)
{
  /* See if the mobot serial id already exists */
  if(!entryExists(mobot->mobot.serialID)) {
    addEntry(mobot->mobot.serialID);
  }
  return setMobotBySerialID(mobot->mobot.serialID, mobot);
}

int CRobotManager::disconnectAll()
{
  int i;
  for(i = 0; i < MAX_CONNECTED; i++) {
    disconnect(i);
  }
  return 0;
}

void CRobotManager::moveMobot(int destIndex, int srcIndex)
{
  _mobots[destIndex] = _mobots[srcIndex];
  _mobots[srcIndex] = NULL;
}

int CRobotManager::moveEntryUp(int index)
{
  int rc;
  if(rc = ConfigFile::moveEntryUp(index)) {
    return rc;
  }
  /* Just swap the data of this entry with the one above it */
  recordMobot_t* tmp;
  tmp = _mobots[index-1];
  _mobots[index-1] = _mobots[index];
  _mobots[index] = tmp;
  return 0;
}

int CRobotManager::moveEntryDown(int index)
{
  int rc;
  if(rc = ConfigFile::moveEntryDown(index)) {
    return rc;
  }
  /* Swap with entry below */
  recordMobot_t *tmp;
  tmp = _mobots[index+1];
  _mobots[index+1] = _mobots[index];
  _mobots[index] = tmp;
  return 0;
}

int CRobotManager::insertEntry(const char* entry, int index)
{
  int rc;
  if(rc = ConfigFile::insertEntry(entry, index)) {
    return rc;
  }
  /* Move the existing mobot array */
  int i;
  for(i = numEntries(); i >= index; i--) {
    _mobots[i+1] = _mobots[i];
  }
  _mobots[index] = NULL;
  return 0;
}

bool CRobotManager::isPlaying()
{
  return _isPlaying;
}

int CRobotManager::connectIndex(int index)
{
  if(isConnected(index)) {
    return 0;
  }
  char name[80];
  sprintf(name, "mobot%d", numConnected()+1);
  int err;
  if(_mobots[index] == NULL) {
    _mobots[index] = RecordMobot_new();
  }
  RecordMobot_init(_mobots[index], name);
  err = RecordMobot_connectWithAddress( _mobots[index], getEntry(index), 1 );
  return err;
}

int CRobotManager::disconnect(int index)
{
  if(_mobots[index] == NULL) {
    return -1;
  }
  /* First check to see how the robot is connected. If it is a TTY connection,
   * we just want to remove the robot from the list of connected robots, but we
   * don't actually want to disconnect. */
  if(((mobot_t*)_mobots[index])->connectionMode != MOBOTCONNECT_TTY) {
    Mobot_disconnect((mobot_t*)_mobots[index]);
  }
  //free(_mobots[index]);
  _mobots[index] = NULL;
  return 0;
}

recordMobot_t* CRobotManager::getUnboundMobot()
{
  int i;
  recordMobot_t* mobot;
  mobot = NULL;
  for(i = 0; i < numEntries(); i++) {
    if(_mobots[i] == NULL) {
      continue;
    }
    if(_mobots[i]->bound == false) {
      mobot = _mobots[i];
      break;
    }
  }
  return mobot;
}

int CRobotManager::numConnected()
{
  int num = 0, i;
  for(i = 0; i < MAX_CONNECTED; i++) {
    if(_mobots[i] == NULL) {
      continue;
    }
    if(_mobots[i] != NULL) {
      num++;
    }
  }
  return num;
}

int CRobotManager::numAvailable()
{
	return numEntries() - numConnected();
}

void CRobotManager::record()
{
  int i;
  recordMobot_t* mobot;
  for(i = 0; i < numConnected(); i++) {
    mobot = getMobot(i);
    RecordMobot_record(mobot);
  }
}

void CRobotManager::deletePose(int pose)
{
  int i;
  recordMobot_t* mobot;
  for(i = 0; i < numConnected(); i++) {
    mobot = getMobot(i);
    RecordMobot_removeMotion(mobot, pose, TRUE);
  }
}

int CRobotManager::remove(int index)
{
  int rc;
  if(rc = ConfigFile::remove(index)) {
    return rc;
  }
  /* Adjust the list of mobots */
  _tmpMobot = _mobots[index];
  int i;
  for(i = index; i < numEntries(); i++) {
    _mobots[i] = _mobots[i+1];
  }
  return 0;
}

void CRobotManager::restoreSavedMobot(int index)
{
  if(_mobots[index] != NULL) {
    free(_mobots[index]);
  }
  _mobots[index] = _tmpMobot;
  _tmpMobot = NULL;
}

void CRobotManager::addDelay(double seconds)
{
  int i;
  recordMobot_t* mobot;
  for(i = 0; i < numConnected(); i++) {
    mobot = getMobot(i);
    RecordMobot_addDelay(mobot, seconds);
  }
}

void* robotManagerPlayThread(void* arg)
{
  CRobotManager *rm = (CRobotManager*)arg;
  recordMobot_t* mobot;
  if(rm->numConnected() <= 0) {
    rm->_isPlaying = false;
    return NULL;
  }
  int index, i;
  /* Go through each motion */
  for(index = 0; index < rm->getMobot(0)->numMotions; index++) {
    /* Go through each mobot */
    for(i = 0; i < rm->numConnected(); i++) {
      mobot = rm->getMobot(i);
      if(RecordMobot_getMotionType(mobot, index) == MOTION_SLEEP) {
        /* Sleep the correct amount of time and break */
        RecordMobot_play(mobot, index);
        break;
      } else if (RecordMobot_getMotionType(mobot, index) == MOTION_POS) {
        RecordMobot_play(mobot, index);
      } else {
        fprintf(stderr, "MEMORY ERROR %s:%d\n", __FILE__, __LINE__);
        rm->_isPlaying = false;
        return NULL;
      }
    }
  }
  rm->_isPlaying = false;
}

void CRobotManager::play()
{
  _isPlaying = true;
  THREAD_T thread;
  THREAD_CREATE(&thread, robotManagerPlayThread, this);
}

recordMobot_t* CRobotManager::getMobot(int connectIndex)
{
	if(connectIndex < 0 || connectIndex >= numConnected()) {
		return NULL;
	}
  int i;
  for(i = 0; i <= MAX_CONNECTED ; i++ ) {
    if(_mobots[i] == NULL) {continue;}
    if((_mobots[i] != NULL) && (_mobots[i]->connectStatus == RMOBOT_CONNECTED)) {
      connectIndex--;
    }
    if(connectIndex < 0) {
      break;
    }
  }
	return _mobots[i];
}

int CRobotManager::setMobotByIndex(int index, recordMobot_t* mobot)
{
  _mobots[index] = mobot;
}

int CRobotManager::setMobotBySerialID(const char* id, recordMobot_t* mobot)
{
  /* Figure out the index, if it is in here */
  int i;
  const char* entry;
  for(i = 0; i < MAX_CONNECTED; i++) {
    entry = getEntry(i);
    if(entry == NULL) { continue; }
    if(!strcasecmp(entry, id)) {
      _mobots[i] = mobot;
      if(_mobots[i]->mobot.connected) {
        char name[80];
        sprintf(_mobots[i]->name, "mobot%d", numConnected()+1);
      }
      return 0;
    }
  }
  return -1;
}

recordMobot_t* CRobotManager::getMobotIndex(int index)
{
  return _mobots[index];
}

recordMobot_t* CRobotManager::getMobotZBAddr(uint16_t addr)
{
  /* See if any of our robots has the zigbee address and return it. */
  int i;
  for(i = 0; i < MAX_CONNECTED; i++) {
    if(_mobots[i] == NULL) {
      continue;
    }
    if(_mobots[i]->mobot.zigbeeAddr == addr) {
      return _mobots[i];
    }
  }
  return NULL;
}

void CRobotManager::connectZBAddr(uint16_t addr)
{
  int i;
  for(i = 0; i < MAX_CONNECTED; i++) {
    if(_mobots[i] == NULL) {
      continue;
    }
    if(_mobots[i]->mobot.zigbeeAddr == addr) {
      if(_mobots[i]->mobot.connected) {
        return;
      }
      connectIndex(i);
      return;
    }
  }
}

int CRobotManager::connectSerialID(const char* id)
{
  /* Figure out the index, if it is in here */
  int i;
  const char* entry;
  for(i = 0; i < MAX_CONNECTED; i++) {
    entry = getEntry(i);
    if(entry == NULL) { continue; }
    if(!strcasecmp(entry, id)) {
      return connectIndex(i);
    }
  }
  return -1;
}

string* CRobotManager::generateChProgram(bool looped, bool holdOnExit)
{
  string buf;
  string *program = new string();
  char tbuf[200];
  int i, j;
  char tmp[80];
  *program += "/* Program generated by BaroboLink */\n";

  for(i = 0; i < numConnected(); i++) {
    if(
        (getMobot(i)->mobot.formFactor == MOBOTFORM_I) ||
        (getMobot(i)->mobot.formFactor == MOBOTFORM_L) 
      ) 
    {
      *program += "#include <linkbot.h>\n\n";
      break;
    }
  }

  for(i = 0; i < numConnected(); i++) {
    if(
        (getMobot(i)->mobot.formFactor != MOBOTFORM_I) &&
        (getMobot(i)->mobot.formFactor != MOBOTFORM_L) 
      ) 
    {
      *program += "#include <mobot.h>\n\n";
      break;
    }
  }

  //*program += "int main() {\n";

  /* Declare the appropiate number of CMobot variables */
  for(i = 0; i < numConnected(); i++) {
    if(getMobot(i)->mobot.formFactor == MOBOTFORM_I) {
      sprintf(tbuf, "CLinkbotI robot%d;\n", i+1);
    } else if (getMobot(i)->mobot.formFactor == MOBOTFORM_L) {
      sprintf(tbuf, "CLinkbotL robot%d;\n", i+1);
    } else {
      sprintf(tbuf, "CMobot robot%d;\n", i+1);
    }
    buf.assign(tbuf);
    *program += buf;
  }

  /* Connect to each one */
  for(i = 0; i < numConnected(); i++) {
    if(getMobot(i)) {
    }
    sprintf(tbuf, "robot%d.connect();\n", i+1);
    buf.assign(tbuf);
    *program += buf;
  }

  if(looped) {
    *program += "/* Set the \"num\" variable to the number of times to loop. */\n";
    *program += "int num = 3;\n";
    *program += "int i;\n";
    *program += "for(i = 0; i < num; i++) {";
  }
  /* Make sure there were connected mobots */
  if(getMobot(0) != NULL) {
    /* Set all of the robot names so they are in order */
    for(j = 0; j < numConnected(); j++) {
      sprintf(tmp, "robot%d", j+1);
      RecordMobot_setName(getMobot(j), tmp);
    }
    /* Now go through each motion */
    for(i = 0; i < getMobot(0)->numMotions; i++) {
      *program += "\n";
      /* First, print the comment for the motion */
      sprintf(tbuf, "/* %s */\n", RecordMobot_getMotionName(getMobot(0), i));
      buf.assign(tbuf);
      //*program += "    ";
      if(looped) *program += "    ";
      *program += buf;
      /* Now, print each robots motion */
      for(j = 0; j < numConnected(); j++) {
        if(numConnected() == 1) {
          RecordMobot_getChMotionStringB(getMobot(j), i, tbuf);
        } else {
          RecordMobot_getChMotionString(getMobot(j), i, tbuf);
        }
        buf.assign(tbuf);
        buf += "\n";
        if(looped) *program += "    ";
        *program += buf;
        if(RecordMobot_getMotionType(getMobot(j), i) == MOTION_SLEEP) {
          break;
        }
      }
      /* Make sure all the robots are done moving */
      for(j = 0; j < numConnected(); j++) {
        if(RecordMobot_getMotionType(getMobot(j), i) == MOTION_SLEEP) {
          break;
        }
        if(numConnected() > 1) {
          sprintf(tbuf, "robot%d.moveWait();\n", j+1);
          buf.assign(tbuf);
          if(looped) *program += "    ";
          *program += buf;
        }
      }
    }
  }
  if(looped) {
    *program += "}\n";
  }

  if(holdOnExit) {
    for(i = 0; i < numConnected(); i++) {
      sprintf(tbuf, "robot%d.setExitState(ROBOT_HOLD);\n", i+1);
      buf.assign(tbuf);
      *program += buf;
    }
  }

  //*program += "return 0;\n";
  //*program += "}\n";
  *program += "\n";
  return program;
}

string* CRobotManager::generateCppProgram(bool looped, bool holdOnExit)
{
  string buf;
  string *program = new string();
  char tbuf[200];
  int i, j;
  char tmp[80];
  *program += "/* Program generated by BaroboLink */\n";

  for(i = 0; i < numConnected(); i++) {
    if(
        (getMobot(i)->mobot.formFactor == MOBOTFORM_I) ||
        (getMobot(i)->mobot.formFactor == MOBOTFORM_L) 
      ) 
    {
      *program += "#include <linkbot.h>\n\n";
      break;
    }
  }

  for(i = 0; i < numConnected(); i++) {
    if(
        (getMobot(i)->mobot.formFactor != MOBOTFORM_I) &&
        (getMobot(i)->mobot.formFactor != MOBOTFORM_L) 
      ) 
    {
      *program += "#include <mobot.h>\n\n";
      break;
    }
  }

  *program += "int main() {\n";

  /* Declare the appropiate number of CMobot variables */
  for(i = 0; i < numConnected(); i++) {
    if(getMobot(i)->mobot.formFactor == MOBOTFORM_I) {
      sprintf(tbuf, "    CLinkbotI robot%d;\n", i+1);
    } else if (getMobot(i)->mobot.formFactor == MOBOTFORM_L) {
      sprintf(tbuf, "    CLinkbotL robot%d;\n", i+1);
    } else {
      sprintf(tbuf, "    CMobot robot%d;\n", i+1);
    }
    buf.assign(tbuf);
    *program += buf;
  }

  /* Connect to each one */
  for(i = 0; i < numConnected(); i++) {
    if(getMobot(i)) {
    }
    sprintf(tbuf, "    robot%d.connect();\n", i+1);
    buf.assign(tbuf);
    *program += buf;
  }

  if(looped) {
    *program += "    /* Set the \"num\" variable to the number of times to loop. */\n";
    *program += "    int num = 3;\n";
    *program += "    int i;\n";
    *program += "    for(i = 0; i < num; i++) {";
  }
  /* Make sure there were connected mobots */
  if(getMobot(0) != NULL) {
    /* Set all of the robot names so they are in order */
    for(j = 0; j < numConnected(); j++) {
      sprintf(tmp, "    robot%d", j+1);
      RecordMobot_setName(getMobot(j), tmp);
    }
    /* Now go through each motion */
    for(i = 0; i < getMobot(0)->numMotions; i++) {
      *program += "\n";
      /* First, print the comment for the motion */
      sprintf(tbuf, "/* %s */\n", RecordMobot_getMotionName(getMobot(0), i));
      buf.assign(tbuf);
      *program += "    ";
      if(looped) *program += "    ";
      *program += buf;
      /* Now, print each robots motion */
      for(j = 0; j < numConnected(); j++) {
        if(numConnected() == 1) {
          RecordMobot_getChMotionStringB(getMobot(j), i, tbuf);
        } else {
          RecordMobot_getChMotionString(getMobot(j), i, tbuf);
        }
        buf.assign(tbuf);
        buf += "\n";
        if(looped) *program += "    ";
        *program += buf;
        if(RecordMobot_getMotionType(getMobot(j), i) == MOTION_SLEEP) {
          break;
        }
      }
      /* Make sure all the robots are done moving */
      for(j = 0; j < numConnected(); j++) {
        if(RecordMobot_getMotionType(getMobot(j), i) == MOTION_SLEEP) {
          break;
        }
        if(numConnected() > 1) {
          sprintf(tbuf, "robot%d.moveWait();\n", j+1);
          buf.assign(tbuf);
          if(looped) *program += "    ";
          *program += buf;
        }
      }
    }
  }
  if(looped) {
    *program += "    }\n";
  }

  if(holdOnExit) {
    for(i = 0; i < numConnected(); i++) {
      sprintf(tbuf, "    robot%d.setExitState(ROBOT_HOLD);\n", i+1);
      buf.assign(tbuf);
      *program += buf;
    }
  }

  *program += "    return 0;\n";
  *program += "}\n";
  *program += "\n";
  return program;
}

string* CRobotManager::generatePythonProgram(bool looped, bool holdOnExit)
{
  string buf;
  string *program = new string();
  char tbuf[200];
  int i, j;
  char tmp[80];
  *program += "#!/usr/bin/env python\n";
  *program += "# Program generated by BaroboLink\n";
  *program += "from barobo.linkbot import *\n";
  *program += "import time\n";

  //*program += "int main() {\n";

  /* Declare the appropiate number of CMobot variables */
  for(i = 0; i < numConnected(); i++) {
    if(getMobot(i)->mobot.formFactor == MOBOTFORM_I) {
      sprintf(tbuf, "robot%d = Linkbot()\n", i+1);
    } else if (getMobot(i)->mobot.formFactor == MOBOTFORM_L) {
      sprintf(tbuf, "robot%d = Linkbot()\n", i+1);
    } else {
      sprintf(tbuf, "robot%d = Mobot();\n", i+1);
    }
    buf.assign(tbuf);
    *program += buf;
  }

  /* Connect to each one */
  for(i = 0; i < numConnected(); i++) {
    if(getMobot(i)) {
    }
    sprintf(tbuf, "robot%d.connect()\n", i+1);
    buf.assign(tbuf);
    *program += buf;
  }

  if(looped) {
    *program += "# Set the \"num\" variable to the number of times to loop\n";
    *program += "num = 3\n";
    *program += "for i in range (0, num):";
  }
  /* Make sure there were connected mobots */
  if(getMobot(0) != NULL) {
    /* Set all of the robot names so they are in order */
    for(j = 0; j < numConnected(); j++) {
      sprintf(tmp, "robot%d", j+1);
      RecordMobot_setName(getMobot(j), tmp);
    }
    /* Now go through each motion */
    for(i = 0; i < getMobot(0)->numMotions; i++) {
      *program += "\n";
      /* First, print the comment for the motion */
      sprintf(tbuf, "# %s \n", RecordMobot_getMotionName(getMobot(0), i));
      buf.assign(tbuf);
      //*program += "    ";
      if(looped) *program += "    ";
      *program += buf;
      /* Now, print each robots motion */
      for(j = 0; j < numConnected(); j++) {
        if(numConnected() == 1) {
          RecordMobot_getPythonMotionStringB(getMobot(j), i, tbuf);
        } else {
          RecordMobot_getPythonMotionString(getMobot(j), i, tbuf);
        }
        buf.assign(tbuf);
        buf += "\n";
        if(looped) *program += "    ";
        *program += buf;
        if(RecordMobot_getMotionType(getMobot(j), i) == MOTION_SLEEP) {
          break;
        }
      }
      /* Make sure all the robots are done moving */
      for(j = 0; j < numConnected(); j++) {
        if(RecordMobot_getMotionType(getMobot(j), i) == MOTION_SLEEP) {
          break;
        }
        if(numConnected() > 1) {
          sprintf(tbuf, "robot%d.moveWait()\n", j+1);
          buf.assign(tbuf);
          if(looped) *program += "    ";
          *program += buf;
        }
      }
    }
  }

  if(holdOnExit) {
    for(i = 0; i < numConnected(); i++) {
      sprintf(tbuf, "robot%d.setExitState(ROBOT_HOLD)\n", i+1);
      buf.assign(tbuf);
      *program += buf;
    }
  }

  //*program += "return 0;\n";
  //*program += "}\n";
  *program += "\n";
  return program;
}

void box_insert_line(GtkBox *vbox, const char *str)
{
  GtkWidget *label = gtk_label_new(str);
  gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(label), TRUE, TRUE, 0);
}

void CRobotManager::generateInteractivePythonProgram(GtkBox *vbox, bool looped, bool holdOnExit)
{
  string buf;
  string *program = new string();
  char tbuf[200];
  int i, j;
  char tmp[80];
  box_insert_line(vbox, "#!/usr/bin/env python");
  box_insert_line(vbox, "# Program generated by BaroboLink");
  box_insert_line(vbox, "from barobo.linkbot import *");
  box_insert_line(vbox, "import time");

  //*program += "int main() {\n";

  /* Declare the appropiate number of CMobot variables */
  for(i = 0; i < numConnected(); i++) {
    if(getMobot(i)->mobot.formFactor == MOBOTFORM_I) {
      sprintf(tbuf, "robot%d = Linkbot()", i+1);
    } else if (getMobot(i)->mobot.formFactor == MOBOTFORM_L) {
      sprintf(tbuf, "robot%d = Linkbot()", i+1);
    } else {
      sprintf(tbuf, "robot%d = Mobot();", i+1);
    }
    buf.assign(tbuf);
    //*program += buf;
    box_insert_line(vbox, buf.c_str());
  }

  /* Connect to each one */
  for(i = 0; i < numConnected(); i++) {
    if(getMobot(i)) {
    }
    sprintf(tbuf, "robot%d.connect()", i+1);
    buf.assign(tbuf);
    box_insert_line(vbox, buf.c_str());
  }

  if(looped) {
    box_insert_line(vbox, "# Set the \"num\" variable to the number of times to loop");
    box_insert_line(vbox, "num = 3");
    box_insert_line(vbox, "for i in range (0, num):");
  }
  /* Make sure there were connected mobots */
  if(getMobot(0) != NULL) {
    /* Set all of the robot names so they are in order */
    for(j = 0; j < numConnected(); j++) {
      sprintf(tmp, "robot%d", j+1);
      RecordMobot_setName(getMobot(j), tmp);
    }
    /* Now go through each motion */
    GtkHBox *box;
    GtkWidget *widget;
    for(i = 0; i < getMobot(0)->numMotions; i++) {
      //box_insert_line(vbox, "\n");
      /* First, print the comment for the motion */
      box = GTK_HBOX(gtk_hbox_new(FALSE, 0));
      sprintf(tbuf, "# %s ", RecordMobot_getMotionName(getMobot(0), i));
      buf.assign(tbuf);
      //*program += "    ";
      if(looped) buf = string("    ") + buf;
      box_insert_line(GTK_BOX(box), buf.c_str());

      /* Now pack a "close" button next to the pose name */
      GtkWidget *image;
      image = gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_SMALL_TOOLBAR);
      gtk_widget_show(image);
      widget = gtk_button_new();
      gtk_button_set_image(GTK_BUTTON(widget), image);
      gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);
      g_signal_connect(
          G_OBJECT(widget), 
          "clicked", 
          G_CALLBACK(on_button_pose_close_clicked), 
          (void*)i);

      gtk_widget_show_all(GTK_WIDGET(box));
      gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(box), TRUE, TRUE, 0);
      /* Now, print each robots motion */
      for(j = 0; j < numConnected(); j++) {
        if(numConnected() == 1) {
          //RecordMobot_getPythonMotionStringB(getMobot(j), i, tbuf);
          RecordMobot_getPythonInteractiveMotionStringB(GTK_WIDGET(vbox), getMobot(j), i);
        } else {
          //RecordMobot_getPythonMotionString(getMobot(j), i, tbuf);
          RecordMobot_getPythonInteractiveMotionString(GTK_WIDGET(vbox), getMobot(j), i);
        }
        buf.assign(tbuf);
        buf += "\n";
        if(looped) *program += "    ";
        *program += buf;
        if(RecordMobot_getMotionType(getMobot(j), i) == MOTION_SLEEP) {
          break;
        }
      }
      /* Make sure all the robots are done moving */
      for(j = 0; j < numConnected(); j++) {
        if(RecordMobot_getMotionType(getMobot(j), i) == MOTION_SLEEP) {
          break;
        }
        if(numConnected() > 1) {
          sprintf(tbuf, "robot%d.moveWait()\n", j+1);
          buf.assign(tbuf);
          if(looped) *program += "    ";
          *program += buf;
        }
      }
    }
  }
#if 0

  if(holdOnExit) {
    for(i = 0; i < numConnected(); i++) {
      sprintf(tbuf, "robot%d.setExitState(ROBOT_HOLD)\n", i+1);
      buf.assign(tbuf);
      *program += buf;
    }
  }

  //*program += "return 0;\n";
  //*program += "}\n";
  *program += "\n";
  return program;
#endif
}

void on_button_pose_close_clicked(GtkWidget *widget, void* userdata)
{
  g_robotManager->deletePose((long int)userdata);
  teachingDialog_refreshRecordedMotions(-1);
}
