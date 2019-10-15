#include "numberedit2.h"
#include "strhelpers.h"


NumberEdit2::NumberEdit2(Window * parent, const rect_t & rect,
    int32_t vmin, int32_t vmax, const char* label, const char * values, int buttonWidth,
    std::function<int32_t()> getValue, std::function<void(int32_t)> setValue, LcdFlags flags) :
    NumberEdit2(parent, rect, vmin, vmax, vmin, vmax, label, values, buttonWidth, getValue, getValue, setValue, setValue, flags)
{
}

NumberEdit2::NumberEdit2(Window * parent, const rect_t & rect, int32_t vmin, int32_t vmax, int32_t minChoice, int32_t maxChoice,
    const char* label, const char * values, int buttonWidth,
    std::function<int32_t()> getValue, std::function<int32_t()> getValueChoice,
    std::function<void(int32_t)> setValue, std::function<void(int32_t)> setValueChoice, LcdFlags flags  ) :

    NumberEdit(parent, { rect.x, rect.y, rect.w - buttonWidth, rect.h }, vmin, vmax, getValue, setValue, flags),
    ChoiceBase(values, minChoice, maxChoice, getValueChoice, setValueChoice, flags)
{

  textButton = new TextButton(parent, { rect.x + rect.w - buttonWidth, rect.y, buttonWidth, rect.h }, std::string(label));
  textButton->setPressHandler(std::bind(&NumberEdit2::onButtonCheck, this));
  int32_t value = getValue();
  //set checked if value out of numberedit range
  checked = (value > NumberEdit::vmax) || (value < NumberEdit::vmin);
  textButton->check(checked);
}

bool NumberEdit2::onTouchEnd(coord_t x, coord_t y) {
  if(checked) return ChoiceBase::handleTouchEnd(x,y);
  return NumberEdit::onTouchEnd(x,y);
}

void NumberEdit2::paint(BitmapBuffer * dc) {
  if(checked) ChoiceBase::paintChoice(dc, this->hasFocus(), rect);
  else NumberEdit::paint(dc);
}

uint8_t NumberEdit2::onButtonCheck() {
  checked = !textButton->checked();
  setOutputType();
  return checked;
}

void NumberEdit2::setOutputType() {
  if (checked) ChoiceBase::setValue((ChoiceBase::vmin + ChoiceBase::vmax)/2);
  else NumberEdit::setValue((NumberEdit::vmin + NumberEdit::vmax)/2);
  invalidate();
}

#if defined(GVARS)
GvarNumberEdit::GvarNumberEdit(Window * parent, const rect_t & rect, int32_t vmin, int32_t vmax,
    std::function<int32_t()> getValue, std::function<void(int32_t)> setValue, LcdFlags flags) :
    NumberEdit2(parent, rect, vmin, vmax, -MAX_GVARS, MAX_GVARS-1, TR_GV, nullptr, 40,
        getValue, std::bind(&GvarNumberEdit::getGVarIndex, this),
        setValue, std::bind(&GvarNumberEdit::setValueFromGVarIndex, this, std::placeholders::_1), flags),
    setValueDirect(std::move(setValue)),
    delta(GV_GET_GV1_VAL(vmin, vmax)),
    flags(flags)
    {
    ChoiceBase::setTextHandler(GvarNumberEdit::getGVarName);
}

int16_t GvarNumberEdit::getGVarIndex() {
  return (int16_t)GV_INDEX_CALC_DELTA(NumberEdit::getValue(), delta);
}

//set value from GVAR index
void GvarNumberEdit::setValueFromGVarIndex(int32_t idx) {
  int16_t value = 0;
  if (idx < 0) {
    value = (int16_t) GV_CALC_VALUE_IDX_NEG(idx, delta);
  } else {
    value = (int16_t) GV_CALC_VALUE_IDX_POS(idx, delta);
  }
  setValueDirect(value);
}

bool GvarNumberEdit::isGvarValue(int value) {
  return (value > NumberEdit::vmax) || (value < NumberEdit::vmin);
}

std::string GvarNumberEdit::getGVarName(int32_t idx) {
  char buffer[16];
  char* pos = buffer;
  if (idx < 0) {
    *pos++ = '-';
    idx = -idx-1;
  }

  if (ZEXIST(g_model.gvars[idx].name))
    zchar2str(pos, g_model.gvars[idx].name, LEN_GVAR_NAME);
  else
    strAppendStringWithIndex(pos, TR_GV, idx+1);

  return std::string(buffer);
}

void GvarNumberEdit::setOutputType() {

  if (checked) {
    //Set first GVAR
    //use direct call because numedit is limiting setter to min max vale
    setValueDirect(delta);
  }
  else{
    int32_t value = NumberEdit::getValue();
    //convert from GVAR to value represented by GVAR
    if(GV_IS_GV_VALUE(value, NumberEdit::vmin, NumberEdit::vmax)) {
      value = GET_GVAR(value, NumberEdit::vmin, NumberEdit::vmax, mixerCurrentFlightMode);
      if(flags & PREC1) value*=10;
    }
    else {
      //in old implementation delta was used
      value = delta;
    }
    NumberEdit::setValue(value);
  }
  invalidate();
}
#endif

