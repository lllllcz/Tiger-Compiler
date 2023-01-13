#include "tiger/frame/temp.h"

#include <cstdio>
#include <set>
#include <sstream>
#include <algorithm>

namespace temp {

LabelFactory LabelFactory::label_factory;
TempFactory TempFactory::temp_factory;

Label *LabelFactory::NewLabel() {
  char buf[100];
  sprintf(buf, "L%d", label_factory.label_id_++);
  return NamedLabel(std::string(buf));
}

/**
 * Get symbol of a label_. The label_ will be created only if it is not found.
 * @param s label_ string
 * @return symbol
 */
Label *LabelFactory::NamedLabel(std::string_view s) {
  return sym::Symbol::UniqueSymbol(s);
}

std::string LabelFactory::LabelString(Label *s) { return s->Name(); }

Temp *TempFactory::NewTemp() {
  Temp *p = new Temp(temp_factory.temp_id_++);
  std::stringstream stream;
  stream << 't';
  stream << p->num_;
  Map::Name()->Enter(p, new std::string(stream.str()));

  return p;
}

int Temp::Int() const { return num_; }

Map *Map::Empty() { return new Map(); }

Map *Map::Name() {
  static Map *m = nullptr;
  if (!m)
    m = Empty();
  return m;
}

Map *Map::LayerMap(Map *over, Map *under) {
  if (over == nullptr)
    return under;
  else
    return new Map(over->tab_, LayerMap(over->under_, under));
}

void Map::Enter(Temp *t, std::string *s) {
  assert(tab_);
  tab_->Enter(t, s);
}

std::string *Map::Look(Temp *t) {
  std::string *s;
  assert(tab_);
  s = tab_->Look(t);
  if (s)
    return s;
  else if (under_)
    return under_->Look(t);
  else
    return nullptr;
}

void Map::DumpMap(FILE *out) {
  tab_->Dump([out](temp::Temp *t, std::string *r) {
    fprintf(out, "t%d -> %s\n", t->Int(), r->data());
  });
  if (under_) {
    fprintf(out, "---------\n");
    under_->DumpMap(out);
  }
}


bool TempList::Contain(temp::Temp *temp) const {
  auto flag_iter = std::find(temp_list_.begin(), temp_list_.end(), temp);
  return flag_iter != temp_list_.end();
}

bool TempList::Equal(temp::TempList *another) const {
  return std::equal(temp_list_.begin(), temp_list_.end(), another->GetList().begin());
}

temp::TempList *TempList::Intersect(temp::TempList *another) const {
  auto res = new temp::TempList();
  for (auto t : temp_list_) {
    if (another->Contain(t)) {
      res->Append(t);
    }
  }
  return res;
}

temp::TempList *TempList::Union(temp::TempList *another) const {
  auto res = new temp::TempList();
  res->setList(temp_list_);
  for (auto t : another->temp_list_) {
    if (!this->Contain(t)) {
      res->Append(t);
    }
  }
  return res;
}

temp::TempList *TempList::Diff(temp::TempList *another) const {
  auto res = new temp::TempList();
  for (auto t : temp_list_) {
    if (!another->Contain(t)) {
      res->Append(t);
    }
  }
  return res;
}

void TempList::replaceTemp(temp::Temp *old_temp, temp::Temp *new_temp) {
  auto iter = std::find(temp_list_.begin(), temp_list_.end(), old_temp);
  *iter = new_temp;
}

void TempList::setList(std::list<Temp *> list) {
  temp_list_ = list;
}

} // namespace temp