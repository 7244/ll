#include <WITCH/WITCH.h>

#define set_LevelCount 8
/*
  0 - new word
  1 - word trying to enter momentary memory
  2 - word is in momentery memory
  3 - word is in before sleep memory
  4 - word is in memory that can corrupt in few sleeps
  5 - word is in memory that can corrupt after weeks of sleep
  6 - word is in permanent-like memory
  7 - word is in permanent memory
*/
uint32_t LevelWaitSeconds[set_LevelCount] = {
  0,
  15,
  40,
  180,
  600,
  6000,
  86400,
  1123200
};
uint32_t LevelHardnessPoints[set_LevelCount] = {
  0,
  2000,
  1500,
  500,
  50,
  12,
  4,
  0
};
uint32_t LevelComboNeeded[set_LevelCount] = {
  0,
  2,
  2,
  2,
  4,
  8,
  8,
  (uint32_t)-1
};


#include <WITCH/PR/PR.h>

#include <WITCH/EV/EV.h>

#include <WITCH/IO/IO.h>
#include <WITCH/IO/print.h>

void print(const char *format, ...){
  IO_fd_t fd_stdout;
  IO_fd_set(&fd_stdout, FD_OUT);
  va_list argv;
  va_start(argv, format);
  IO_vprint(&fd_stdout, format, argv);
  va_end(argv);
}

#include <WITCH/T/T.h>

#include <WITCH/RAND/RAND.h>

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>

uint32_t HardnessPointLimitForNewWord = LevelHardnessPoints[1] * 10000;

struct list_t{
  std::string FileName;

  struct io_t{
    bool FirstAnswer = true;
    sintptr_t TrueStreak = 0;
    uint8_t CurrentLevel = 0;
    sint64_t LastAnswerAt = 0;
    std::string i;
    std::string o;
  };

  std::vector<io_t> io_list;
};

struct ioi_t{
  uintptr_t ListIndex;
  uintptr_t IOIndex;
};

struct pile_t{
  uint8_t InputData[0x1000];
  uintptr_t InputSize = 0;
  std::vector<list_t> lists;

  ioi_t cq; // current question

  uint64_t Hardness = 0;
}pile;

bool IsInputDupe(std::string &fn, std::string &i, std::string &o){
  for(uintptr_t il = 0; il < pile.lists.size(); il++){
    list_t *list = &pile.lists[il];
    for(uintptr_t iio = 0; iio < list->io_list.size(); iio++){
      list_t::io_t *io = &list->io_list[iio];
      if(i == io->i){
        print("input \"%s\" \"%s\"=\"%s\" is also in list \"%s\" = \"%s\". will be ignored\n",
          fn.c_str(), i.c_str(), o.c_str(), list->FileName.c_str(), io->o.c_str());
        return true;
      }
    }
  }
  return false;
}

void ReadFileToList(list_t *list){
  std::ifstream f(list->FileName, std::ifstream::in);
  if(!f){
    PR_abort();
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  std::string Data = ss.str();

  list_t::io_t io;

  uint8_t Mode = 0;
  bool Ignore = false;
  uintptr_t LastLetterAt = (uintptr_t)-1;
  uintptr_t BeginLetterAt;
  for(uintptr_t i = 0; i < Data.size(); i++){
    if(Data[i] == '\r'){
      continue;
    }
    else if(Data[i] == '\n'){
      if(Mode == 3){
        io.o = std::string((const char *)&Data[BeginLetterAt], LastLetterAt - BeginLetterAt + 1);

        if(IsInputDupe(list->FileName, io.i, io.o)){
          Ignore = true;
        }

        if(Ignore == false){
          list->io_list.push_back(io);
        }
        Mode = 0;
        Ignore = false;
      }
      else if(Mode != 0){
        PR_abort();
      }
    }
    else if(Data[i] == '='){
      if(Mode != 1){
        print("= need to have something before itself\n");
        PR_abort();
      }
      io.i = std::string((const char *)&Data[BeginLetterAt], LastLetterAt - BeginLetterAt + 1);
      Mode++;
    }
    else if(Data[i] == ' '){

    }
    else{
      if(Mode == 0 || Mode == 2){
        Mode++;
        BeginLetterAt = i;
      }
      LastLetterAt = i;
    }
  }
}

list_t::io_t *GetIOFromIndex(uintptr_t ListIndex, uintptr_t IOIndex){
  list_t *list = &pile.lists[ListIndex];
  return &list->io_list[IOIndex];
}

void AskQuestion(){
  list_t *list = &pile.lists[pile.cq.ListIndex];
  list_t::io_t *io = &list->io_list[pile.cq.IOIndex];

  print("%s: ", io->i.c_str());
}

void GiveAnswer(){
  list_t *list = &pile.lists[pile.cq.ListIndex];
  list_t::io_t *io = &list->io_list[pile.cq.IOIndex];

  print("%s", io->o.c_str());
}

std::vector<ioi_t> SortIndexes(){
  sint64_t ct = T_nowi();

  std::vector<ioi_t> ioi_list;

  for(uintptr_t il = 0; il < pile.lists.size(); il++){
    list_t *list = &pile.lists[il];
    for(uintptr_t iio = 0; iio < list->io_list.size(); iio++){
      list_t::io_t *io = &list->io_list[iio];

      if(io->CurrentLevel == 0){
        continue;
      }

      sint64_t diff = ct - io->LastAnswerAt;
      f64_t s = (f64_t)diff / (f64_t)LevelWaitSeconds[io->CurrentLevel] * 1000000000;
      if(s < 1){
        continue;
      }

      ioi_list.push_back({.ListIndex = il, .IOIndex = iio});
    }
  }

  if(ioi_list.size() == 0){
    for(uintptr_t il = 0; il < pile.lists.size(); il++){
      list_t *list = &pile.lists[il];
      for(uintptr_t iio = 0; iio < list->io_list.size(); iio++){
        list_t::io_t *io = &list->io_list[iio];

        if(io->CurrentLevel == 0){
          continue;
        }

        ioi_list.push_back({.ListIndex = il, .IOIndex = iio});
      }
    }
  }

  std::sort(ioi_list.begin(), ioi_list.end(), [&](ioi_t ioi0, ioi_t ioi1){
    list_t *list0 = &pile.lists[ioi0.ListIndex];
    list_t::io_t *io0 = &list0->io_list[ioi0.IOIndex];
    list_t *list1 = &pile.lists[ioi1.ListIndex];
    list_t::io_t *io1 = &list1->io_list[ioi1.IOIndex];

    sint64_t diff0 = ct - io0->LastAnswerAt;
    sint64_t diff1 = ct - io1->LastAnswerAt;
    f64_t s0 = (f64_t)diff0 / (f64_t)LevelWaitSeconds[io0->CurrentLevel] * 1000000000;
    f64_t s1 = (f64_t)diff1 / (f64_t)LevelWaitSeconds[io1->CurrentLevel] * 1000000000;

    return s0 > s1;
  });

  return ioi_list;
}

void ChangeLevel(uintptr_t il, uintptr_t iio, uint32_t Level){
  list_t *list = &pile.lists[il];
  list_t::io_t *io = &list->io_list[iio];
  pile.Hardness -= LevelHardnessPoints[io->CurrentLevel];
  io->CurrentLevel = Level;
  pile.Hardness += LevelHardnessPoints[io->CurrentLevel];
}

void FillPool(std::vector<ioi_t> &ioi_list){
  auto sorted = SortIndexes();

  if(pile.Hardness < HardnessPointLimitForNewWord) do{
    uintptr_t Total = 0;

    for(uintptr_t il = 0; il < pile.lists.size(); il++){
      list_t *list = &pile.lists[il];
      for(uintptr_t iio = 0; iio < list->io_list.size(); iio++){
        list_t::io_t *io = &list->io_list[iio];
        if(io->CurrentLevel != 0){
          continue;
        }
        Total++;
      }
    }

    if(Total == 0){
      break;
    }
    uintptr_t Selected = RAND_hard_0() % Total;
    Total = 0;

    for(uintptr_t il = 0; il < pile.lists.size(); il++){
      list_t *list = &pile.lists[il];
      for(uintptr_t iio = 0; iio < list->io_list.size(); iio++){
        list_t::io_t *io = &list->io_list[iio];
        if(io->CurrentLevel != 0){
          continue;
        }
        if(Total++ != Selected){
          continue;
        }
        ChangeLevel(il, iio, 1);
        ioi_list.push_back({.ListIndex = il, .IOIndex = iio});
        goto gt_Done;
      }
    }
  }while(0);

  ioi_list.push_back({.ListIndex = sorted[0].ListIndex, .IOIndex = sorted[0].IOIndex});

  gt_Done:;
}

void SetQuestion(){
  std::vector<ioi_t> ioi_list;

  FillPool(ioi_list);

  if(ioi_list.size() == 0){
    print("ioi_list is empty, nothing to do.\n");
    PR_exit(0);
  }

  uintptr_t ioi_index = RAND_hard_32() % ioi_list.size();
  ioi_t *ioi = &ioi_list[ioi_index];

  pile.cq.ListIndex = ioi->ListIndex;
  pile.cq.IOIndex = ioi->IOIndex;

  AskQuestion();
}

void AnswerToQuestion(const void *Data, uintptr_t Size){
  sint64_t ct = T_nowi();

  list_t *list = &pile.lists[pile.cq.ListIndex];
  list_t::io_t *io = &list->io_list[pile.cq.IOIndex];

  f64_t diff_or_answertime = LevelWaitSeconds[io->CurrentLevel];
  if(diff_or_answertime > ct - io->LastAnswerAt){
    diff_or_answertime = ct - io->LastAnswerAt;
  }

  io->LastAnswerAt = ct;
  io->LastAnswerAt -= f64_t(RAND_hard_16() % 25000) / 100000 * diff_or_answertime;
  bool FirstAnswer = io->FirstAnswer;
  if(io->FirstAnswer == true){
    io->FirstAnswer = false;
  }

  if(Size != io->o.size()){
    goto gt_wrong;
  }

  if(STR_ncasecmp(Data, io->o.c_str(), Size)){
    goto gt_wrong;
  }

  if(io->CurrentLevel == 1 && FirstAnswer == true){
    io->TrueStreak = 0;
    ChangeLevel(pile.cq.ListIndex, pile.cq.IOIndex, 7);
  }
  else{
    io->TrueStreak++;
    if(io->TrueStreak >= LevelComboNeeded[io->CurrentLevel]){
      io->TrueStreak = 0;
      ChangeLevel(pile.cq.ListIndex, pile.cq.IOIndex, std::min(io->CurrentLevel + 1, set_LevelCount - 1));
    }
  }

  SetQuestion();

  return;
  gt_wrong:;

  io->TrueStreak--;
  if(io->TrueStreak <= -2){
    ChangeLevel(pile.cq.ListIndex, pile.cq.IOIndex, std::max(io->CurrentLevel - 1, 1));
  }

  print("Wrong. ");
  GiveAnswer();
  print("\n");

  SetQuestion();

  return;
}

bool IsInputLineDone(uint8_t **buffer, IO_ssize_t *size){
  for(uintptr_t i = 0; i < *size; i++){
    if((*buffer)[i] == 0x7f || (*buffer)[i] == 0x08){
      if(pile.InputSize){
        pile.InputSize--;
      }
      continue;
    }
    if((*buffer)[i] == '\n' || (*buffer)[i] == '\r'){
      if((*buffer)[i] == '\r'){
        print("\r\n");
      }
      *buffer += i + 1;
      *size -= i + 1;
      return 1;
    }
    pile.InputData[pile.InputSize++] = (*buffer)[i];
  }
  return 0;
}

void cb_InputEvent(){
  uint8_t _buffer[0x1000];
  uint8_t *buffer = _buffer;

  IO_fd_t fd;
  IO_fd_set(&fd, STDIN_FILENO);
  IO_ssize_t size = IO_read(&fd, buffer, sizeof(_buffer));
  if(size < 0){
    PR_abort();
  }

  while(1){
    if(!IsInputLineDone(&buffer, &size)){
      return;
    }
    if(pile.InputSize != 0 && pile.InputData[0] == '!'){
      print("  Hardness: %llu\n", pile.Hardness);
      print("\n");
      uint64_t LevelInputCount[set_LevelCount];
      for(uintptr_t i = 0; i < set_LevelCount; i++){
        LevelInputCount[i] = 0;
      }
      for(uintptr_t il = 0; il < pile.lists.size(); il++){
        list_t *list = &pile.lists[il];
        for(uintptr_t iio = 0; iio < list->io_list.size(); iio++){
          list_t::io_t *io = &list->io_list[iio];
          LevelInputCount[io->CurrentLevel]++;
        }
      }
      for(uintptr_t i = 0; i < set_LevelCount; i++){
        print("  LevelInputCount[%lu] %llu\n", (uint32_t)i, LevelInputCount[i]);
      }
      print("\n");
      uintptr_t TotalNotSeenWords = LevelInputCount[0];
      uintptr_t TotalSeenWords = 0;
      for(uintptr_t i = 1; i < set_LevelCount; i++){
        TotalSeenWords += LevelInputCount[i];
      }
      print("  TotalNotSeenWords %llu\n", TotalNotSeenWords);
      print("  TotalSeenWords %llu\n", TotalSeenWords);

      AskQuestion();
    }
    else{
      AnswerToQuestion(pile.InputData, pile.InputSize);
    }
    pile.InputSize = 0;
  }
}

int main(){
  for(const auto &dirEntry : std::filesystem::recursive_directory_iterator("lists")){
    pile.lists.push_back({});

    list_t *list = &pile.lists[pile.lists.size() - 1];
    std::string fn = dirEntry.path().string();

    list->FileName = fn;

    ReadFileToList(list);
  }

  SetQuestion();

  while(1){
    cb_InputEvent();
  }

  return 0;
}
