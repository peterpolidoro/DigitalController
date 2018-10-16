// ----------------------------------------------------------------------------
// DigitalController.cpp
//
//
// Authors:
// Peter Polidoro peterpolidoro@gmail.com
// ----------------------------------------------------------------------------
#include "../DigitalController.h"


using namespace digital_controller;

DigitalController::DigitalController()
{
}

DigitalController::~DigitalController()
{
  disableAll();
}

void DigitalController::setup()
{
  // Parent Setup
  ModularDeviceBase::setup();

  // Reset Watchdog
  resetWatchdog();

  // Event Controller Setup
  event_controller_.setup();

  // Pin Setup
  pinMode(constants::enable_pin,OUTPUT);
  enableAll();

  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    pinMode(constants::signal_pins[channel],OUTPUT);
  }
  channels_ = 0;
  setAllChannelsOff();

  // PWM Indexes
  initializePwmIndexes();

  // Pins

  // Set Device ID
  modular_server_.setDeviceName(constants::device_name);

  // Add Hardware

  // Add Firmware
  modular_server_.addFirmware(constants::firmware_info,
    properties_,
    parameters_,
    functions_,
    callbacks_);

  // Properties
  modular_server::Property & channel_count_property = modular_server_.createProperty(constants::channel_count_property_name,constants::channel_count_default);
  channel_count_property.setRange(constants::channel_count_min,constants::CHANNEL_COUNT_MAX);
  channel_count_property.attachPostSetValueFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelCountHandler));

  modular_server::Property & power_max_property = modular_server_.createProperty(constants::power_max_property_name,constants::power_max_default);
  power_max_property.setRange(constants::power_min,constants::power_max);
  power_max_property.setUnits(constants::percent_units);
  power_max_property.attachPostSetElementValueFunctor(makeFunctor((Functor1<size_t> *)0,*this,&DigitalController::setPowerMaxHandler));

  setPowersToMax();

  // Parameters
  modular_server::Parameter & channel_parameter = modular_server_.createParameter(constants::channel_parameter_name);

  modular_server::Parameter & channels_parameter = modular_server_.createParameter(constants::channels_parameter_name);

  modular_server::Parameter & power_parameter = modular_server_.createParameter(constants::power_parameter_name);
  power_parameter.setRange(constants::power_min,constants::power_max);
  power_parameter.setUnits(constants::percent_units);

  modular_server::Parameter & powers_parameter = modular_server_.createParameter(constants::powers_parameter_name);
  powers_parameter.setRange(constants::power_min,constants::power_max);
  powers_parameter.setUnits(constants::percent_units);

  modular_server::Parameter & delay_parameter = modular_server_.createParameter(constants::delay_parameter_name);
  delay_parameter.setRange(constants::delay_min,constants::delay_max);
  delay_parameter.setUnits(constants::ms_units);

  modular_server::Parameter & period_parameter = modular_server_.createParameter(constants::period_parameter_name);
  period_parameter.setRange(constants::period_min,constants::period_max);
  period_parameter.setUnits(constants::ms_units);

  modular_server::Parameter & on_duration_parameter = modular_server_.createParameter(constants::on_duration_parameter_name);
  on_duration_parameter.setRange(constants::on_duration_min,constants::on_duration_max);
  on_duration_parameter.setUnits(constants::ms_units);

  modular_server::Parameter & count_parameter = modular_server_.createParameter(constants::count_parameter_name);
  count_parameter.setRange(constants::count_min,constants::count_max);
  count_parameter.setUnits(constants::ms_units);

  modular_server::Parameter & pwm_index_parameter = modular_server_.createParameter(constants::pwm_index_parameter_name);
  pwm_index_parameter.setRange(constants::pwm_index_min,(long)constants::INDEXED_PWM_COUNT_MAX-1);

  // modular_server::Parameter & delays_parameter = modular_server_.createParameter(constants::delays_parameter_name);
  // delays_parameter.setRange(constants::delay_min,constants::delay_max);
  // delays_parameter.setArrayLengthRange(1,constants::PWM_LEVEL_COUNT_MAX);
  // delays_parameter.setUnits(constants::ms_units);

  modular_server::Parameter & periods_parameter = modular_server_.createParameter(constants::periods_parameter_name);
  periods_parameter.setRange(constants::period_min,constants::period_max);
  periods_parameter.setArrayLengthRange(1,constants::PWM_LEVEL_COUNT_MAX);
  periods_parameter.setUnits(constants::ms_units);

  modular_server::Parameter & on_durations_parameter = modular_server_.createParameter(constants::on_durations_parameter_name);
  on_durations_parameter.setRange(constants::on_duration_min,constants::on_duration_max);
  on_durations_parameter.setArrayLengthRange(1,constants::PWM_LEVEL_COUNT_MAX);
  on_durations_parameter.setUnits(constants::ms_units);

  setChannelCountHandler();

  // Functions
  modular_server::Function & enable_all_function = modular_server_.createFunction(constants::enable_all_function_name);
  enable_all_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::enableAllHandler));

  modular_server::Function & disable_all_function = modular_server_.createFunction(constants::disable_all_function_name);
  disable_all_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::disableAllHandler));

  modular_server::Function & enabled_function = modular_server_.createFunction(constants::enabled_function_name);
  enabled_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::enabledHandler));
  enabled_function.setResultTypeBool();

  modular_server::Function & set_power_when_on_function = modular_server_.createFunction(constants::set_power_when_on_function_name);
  set_power_when_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setPowerWhenOnHandler));
  set_power_when_on_function.addParameter(channel_parameter);
  set_power_when_on_function.addParameter(power_parameter);
  set_power_when_on_function.setResultTypeLong();

  modular_server::Function & set_powers_when_on_function = modular_server_.createFunction(constants::set_powers_when_on_function_name);
  set_powers_when_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setPowersWhenOnHandler));
  set_powers_when_on_function.addParameter(powers_parameter);
  set_powers_when_on_function.setResultTypeArray();
  set_powers_when_on_function.setResultTypeLong();

  modular_server::Function & set_all_powers_when_on_function = modular_server_.createFunction(constants::set_all_powers_when_on_function_name);
  set_all_powers_when_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setAllPowersWhenOnHandler));
  set_all_powers_when_on_function.addParameter(power_parameter);
  set_all_powers_when_on_function.setResultTypeArray();
  set_all_powers_when_on_function.setResultTypeLong();

  modular_server::Function & get_powers_when_on_function = modular_server_.createFunction(constants::get_powers_when_on_function_name);
  get_powers_when_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::getPowersWhenOnHandler));
  get_powers_when_on_function.setResultTypeArray();
  get_powers_when_on_function.setResultTypeLong();

  modular_server::Function & get_powers_function = modular_server_.createFunction(constants::get_powers_function_name);
  get_powers_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::getPowersHandler));
  get_powers_function.setResultTypeArray();
  get_powers_function.setResultTypeLong();

  modular_server::Function & set_channel_on_function = modular_server_.createFunction(constants::set_channel_on_function_name);
  set_channel_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelOnHandler));
  set_channel_on_function.addParameter(channel_parameter);

  modular_server::Function & set_channel_on_at_power_function = modular_server_.createFunction(constants::set_channel_on_at_power_function_name);
  set_channel_on_at_power_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelOnAtPowerHandler));
  set_channel_on_at_power_function.addParameter(channel_parameter);
  set_channel_on_at_power_function.addParameter(power_parameter);

  modular_server::Function & set_channel_off_function = modular_server_.createFunction(constants::set_channel_off_function_name);
  set_channel_off_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelOffHandler));
  set_channel_off_function.addParameter(channel_parameter);

  modular_server::Function & set_channels_on_function = modular_server_.createFunction(constants::set_channels_on_function_name);
  set_channels_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelsOnHandler));
  set_channels_on_function.addParameter(channels_parameter);

  modular_server::Function & set_channels_on_at_power_function = modular_server_.createFunction(constants::set_channels_on_at_power_function_name);
  set_channels_on_at_power_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelsOnAtPowerHandler));
  set_channels_on_at_power_function.addParameter(channels_parameter);
  set_channels_on_at_power_function.addParameter(power_parameter);

  modular_server::Function & set_channels_off_function = modular_server_.createFunction(constants::set_channels_off_function_name);
  set_channels_off_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelsOffHandler));
  set_channels_off_function.addParameter(channels_parameter);

  modular_server::Function & toggle_channel_function = modular_server_.createFunction(constants::toggle_channel_function_name);
  toggle_channel_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::toggleChannelHandler));
  toggle_channel_function.addParameter(channel_parameter);

  modular_server::Function & toggle_channels_function = modular_server_.createFunction(constants::toggle_channels_function_name);
  toggle_channels_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::toggleChannelsHandler));
  toggle_channels_function.addParameter(channels_parameter);

  modular_server::Function & toggle_all_channels_function = modular_server_.createFunction(constants::toggle_all_channels_function_name);
  toggle_all_channels_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::toggleAllChannelsHandler));

  modular_server::Function & set_all_channels_on_function = modular_server_.createFunction(constants::set_all_channels_on_function_name);
  set_all_channels_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setAllChannelsOnHandler));

  modular_server::Function & set_all_channels_off_function = modular_server_.createFunction(constants::set_all_channels_off_function_name);
  set_all_channels_off_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setAllChannelsOffHandler));

  modular_server::Function & set_channel_on_all_others_off_function = modular_server_.createFunction(constants::set_channel_on_all_others_off_function_name);
  set_channel_on_all_others_off_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelOnAllOthersOffHandler));
  set_channel_on_all_others_off_function.addParameter(channel_parameter);

  modular_server::Function & set_channel_off_all_others_on_function = modular_server_.createFunction(constants::set_channel_off_all_others_on_function_name);
  set_channel_off_all_others_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelOffAllOthersOnHandler));
  set_channel_off_all_others_on_function.addParameter(channel_parameter);

  modular_server::Function & set_channels_on_all_others_off_function = modular_server_.createFunction(constants::set_channels_on_all_others_off_function_name);
  set_channels_on_all_others_off_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelsOnAllOthersOffHandler));
  set_channels_on_all_others_off_function.addParameter(channels_parameter);

  modular_server::Function & set_channels_off_all_others_on_function = modular_server_.createFunction(constants::set_channels_off_all_others_on_function_name);
  set_channels_off_all_others_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::setChannelsOffAllOthersOnHandler));
  set_channels_off_all_others_on_function.addParameter(channels_parameter);

  modular_server::Function & channel_is_on_function = modular_server_.createFunction(constants::channel_is_on_function_name);
  channel_is_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::channelIsOnHandler));
  channel_is_on_function.addParameter(channel_parameter);
  channel_is_on_function.setResultTypeBool();

  modular_server::Function & get_channels_on_function = modular_server_.createFunction(constants::get_channels_on_function_name);
  get_channels_on_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::getChannelsOnHandler));
  get_channels_on_function.setResultTypeArray();
  get_channels_on_function.setResultTypeLong();

  modular_server::Function & get_channels_off_function = modular_server_.createFunction(constants::get_channels_off_function_name);
  get_channels_off_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::getChannelsOffHandler));
  get_channels_off_function.setResultTypeArray();
  get_channels_off_function.setResultTypeLong();

  modular_server::Function & get_channel_count_function = modular_server_.createFunction(constants::get_channel_count_function_name);
  get_channel_count_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::getChannelCountHandler));

  modular_server::Function & add_pwm_function = modular_server_.createFunction(constants::add_pwm_function_name);
  add_pwm_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::addPwmHandler));
  add_pwm_function.addParameter(channels_parameter);
  add_pwm_function.addParameter(delay_parameter);
  add_pwm_function.addParameter(period_parameter);
  add_pwm_function.addParameter(on_duration_parameter);
  add_pwm_function.addParameter(count_parameter);
  add_pwm_function.setResultTypeLong();

  modular_server::Function & start_pwm_function = modular_server_.createFunction(constants::start_pwm_function_name);
  start_pwm_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::startPwmHandler));
  start_pwm_function.addParameter(channels_parameter);
  start_pwm_function.addParameter(delay_parameter);
  start_pwm_function.addParameter(period_parameter);
  start_pwm_function.addParameter(on_duration_parameter);
  start_pwm_function.setResultTypeLong();

  modular_server::Function & add_recursive_pwm_function = modular_server_.createFunction(constants::add_recursive_pwm_function_name);
  add_recursive_pwm_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::addRecursivePwmHandler));
  add_recursive_pwm_function.addParameter(channels_parameter);
  add_recursive_pwm_function.addParameter(delay_parameter);
  add_recursive_pwm_function.addParameter(periods_parameter);
  add_recursive_pwm_function.addParameter(on_durations_parameter);
  add_recursive_pwm_function.addParameter(count_parameter);
  add_recursive_pwm_function.setResultTypeLong();

  modular_server::Function & start_recursive_pwm_function = modular_server_.createFunction(constants::start_recursive_pwm_function_name);
  start_recursive_pwm_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::startRecursivePwmHandler));
  start_recursive_pwm_function.addParameter(channels_parameter);
  start_recursive_pwm_function.addParameter(delay_parameter);
  start_recursive_pwm_function.addParameter(periods_parameter);
  start_recursive_pwm_function.addParameter(on_durations_parameter);
  start_recursive_pwm_function.setResultTypeLong();

  modular_server::Function & stop_pwm_function = modular_server_.createFunction(constants::stop_pwm_function_name);
  stop_pwm_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::stopPwmHandler));
  stop_pwm_function.addParameter(pwm_index_parameter);

  modular_server::Function & stop_all_pwm_function = modular_server_.createFunction(constants::stop_all_pwm_function_name);
  stop_all_pwm_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::stopAllPwmHandler));

  modular_server::Function & get_channels_pwm_indexes_function = modular_server_.createFunction(constants::get_channels_pwm_indexes_function_name);
  get_channels_pwm_indexes_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::getChannelsPwmIndexesHandler));
  get_channels_pwm_indexes_function.setResultTypeArray();
  get_channels_pwm_indexes_function.setResultTypeLong();

  modular_server::Function & get_pwm_info_function = modular_server_.createFunction(constants::get_pwm_info_function_name);
  get_pwm_info_function.attachFunctor(makeFunctor((Functor0 *)0,*this,&DigitalController::getPwmInfoHandler));
  get_pwm_info_function.setResultTypeArray();
  get_pwm_info_function.setResultTypeObject();

  // Callbacks

}

void DigitalController::enableAll()
{
  digitalWrite(constants::enable_pin,LOW);
  enabled_ = true;
}

void DigitalController::disableAll()
{
  digitalWrite(constants::enable_pin,HIGH);
  enabled_ = false;
}

bool DigitalController::enabled()
{
  return enabled_;
}

long DigitalController::setPowerWhenOn(size_t channel,
  long power)
{
  long power_to_set = 0;
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    power_to_set = power;
    if (power_to_set < constants::power_min)
    {
      power_to_set = constants::power_min;
    }
    else
    {
      modular_server::Property & power_max_property = modular_server_.property(constants::power_max_property_name);
      long power_max;
      power_max_property.getElementValue(channel,power_max);
      if (power_to_set > power_max)
      {
        power_to_set = power_max;
      }
    }
    noInterrupts();
    powers_when_on_[channel] = power_to_set;
    interrupts();
    updateChannel(channel);
  }
  return power_to_set;
}

long DigitalController::getPowerWhenOn(size_t channel)
{
  long power = constants::power_min;
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    noInterrupts();
    power = powers_when_on_[channel];
    interrupts();
  }
  return power;
}

long DigitalController::getPower(size_t channel)
{
  long power = constants::power_min;
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    noInterrupts();
    power = powers_[channel];
    interrupts();
  }
  return power;
}

void DigitalController::setChannels(uint32_t channels)
{
  uint32_t bit = 1;
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    if ((bit << channel) & channels)
    {
      setChannelOn(channel);
    }
    else
    {
      setChannelOff(channel);
    }
  }
}

void DigitalController::setChannelOn(size_t channel)
{
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    uint32_t bit = 1;
    bit = bit << channel;

    noInterrupts();
    long power = powers_when_on_[channel];
    interrupts();
    if (power == 0)
    {
      setChannelOff(channel);
      return;
    }
    long analog_write_value = powerToAnalogWriteValue(power);

    noInterrupts();
    channels_ |= bit;
    analogWrite(constants::signal_pins[channel],analog_write_value);
    powers_[channel] = power;
    interrupts();
  }
}

void DigitalController::setChannelOnAtPower(size_t channel,
  long power)
{
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    uint32_t bit = 1;
    bit = bit << channel;

    if (power == 0)
    {
      setChannelOff(channel);
      return;
    }
    long analog_write_value = powerToAnalogWriteValue(power);

    noInterrupts();
    channels_ |= bit;
    analogWrite(constants::signal_pins[channel],analog_write_value);
    powers_[channel] = power;
    interrupts();
  }
}

void DigitalController::setChannelOff(size_t channel)
{
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    uint32_t bit = 1;
    bit = bit << channel;

    noInterrupts();
    channels_ &= ~bit;
    analogWrite(constants::signal_pins[channel],constants::analog_write_min);
    powers_[channel] = constants::power_min;
    interrupts();
  }
}

void DigitalController::setChannelsOn(uint32_t channels)
{
  uint32_t bit = 1;
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    if ((bit << channel) & channels)
    {
      setChannelOn(channel);
    }
  }
}

void DigitalController::setChannelsOnAtPower(uint32_t channels,
  long power)
{
  uint32_t bit = 1;
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    if ((bit << channel) & channels)
    {
      setChannelOnAtPower(channel,power);
    }
  }
}

void DigitalController::setChannelsOff(uint32_t channels)
{
  uint32_t bit = 1;
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    if ((bit << channel) & channels)
    {
      setChannelOff(channel);
    }
  }
}

void DigitalController::toggleChannel(size_t channel)
{
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    uint32_t bit = 1;
    bit = bit << channel;
    noInterrupts();
    uint32_t channels = channels_;
    interrupts();
    channels ^= bit;
    if (bit & channels)
    {
      setChannelOn(channel);
    }
    else
    {
      setChannelOff(channel);
    }
  }
}

void DigitalController::toggleChannels(uint32_t channels)
{
  noInterrupts();
  channels_ ^= channels;
  interrupts();
  setChannels(channels_);
}

void DigitalController::toggleAllChannels()
{
  noInterrupts();
  channels_ = ~channels_;
  interrupts();
  setChannels(channels_);
}

void DigitalController::setAllChannelsOn()
{
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    setChannelOn(channel);
  }
}

void DigitalController::setAllChannelsOff()
{
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    setChannelOff(channel);
  }
}

void DigitalController::setChannelOnAllOthersOff(size_t channel)
{
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    uint32_t bit = 1;
    bit = bit << channel;
    noInterrupts();
    channels_ = bit;
    interrupts();
    setChannels(channels_);
  }
}

void DigitalController::setChannelOffAllOthersOn(size_t channel)
{
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    uint32_t bit = 1;
    bit = bit << channel;
    noInterrupts();
    channels_ = ~bit;
    interrupts();
    setChannels(channels_);
  }
}

void DigitalController::setChannelsOnAllOthersOff(uint32_t channels)
{
  noInterrupts();
  channels_ = channels;
  interrupts();
  setChannels(channels_);
}

void DigitalController::setChannelsOffAllOthersOn(uint32_t channels)
{
  noInterrupts();
  channels_ = ~channels;
  interrupts();
  setChannels(channels_);
}

bool DigitalController::channelIsOn(size_t channel)
{
  bool channel_is_on = false;
  if (channel < constants::CHANNEL_COUNT_MAX)
  {
    noInterrupts();
    uint32_t channels = channels_;
    interrupts();
    uint32_t bit = 1;
    if ((bit << channel) & channels)
    {
      channel_is_on = true;
    }
  }
  return channel_is_on;
}

uint32_t DigitalController::getChannelsOn()
{
  return channels_;
}

size_t DigitalController::getChannelCount()
{
  long channel_count;
  modular_server_.property(constants::channel_count_property_name).getValue(channel_count);

  return channel_count;
}

int DigitalController::addPwm(uint32_t channels,
  long delay,
  long period,
  long on_duration,
  long count)
{
  if (indexed_pwm_.full() || (event_controller_.eventsAvailable() < 2))
  {
    return constants::NO_PWM_AVAILABLE_INDEX;
  }
  digital_controller::constants::PwmInfo pwm_info;
  pwm_info.channels = channels;
  pwm_info.running = false;
  pwm_info.level = 0;
  pwm_info.top_level = true;
  pwm_info.child_index = constants::NO_CHILD_PWM_INDEX;
  pwm_info.delay = delay;
  pwm_info.period = period;
  pwm_info.on_duration = on_duration;
  pwm_info.count = count;
  pwm_info.stopped_before_count_completed = false;
  pwm_info.functor_count_completed = functor_dummy_;
  pwm_info.functor_arg = -1;
  int pwm_index = indexed_pwm_.add(pwm_info);
  EventIdPair event_id_pair = event_controller_.addPwmUsingDelay(makeFunctor((Functor1<int> *)0,*this,&DigitalController::setChannelsOnHandler),
    makeFunctor((Functor1<int> *)0,*this,&DigitalController::setChannelsOffHandler),
    delay,
    period,
    on_duration,
    count,
    pwm_index);
  event_controller_.addStartFunctor(event_id_pair,makeFunctor((Functor1<int> *)0,*this,&DigitalController::startPwmHandler));
  event_controller_.addStopFunctor(event_id_pair,makeFunctor((Functor1<int> *)0,*this,&DigitalController::stopPwmHandler));
  indexed_pwm_[pwm_index].event_id_pair = event_id_pair;
  event_controller_.enable(event_id_pair);
  return pwm_index;
}

int DigitalController::startPwm(uint32_t channels,
  long delay,
  long period,
  long on_duration)
{
  return addPwm(channels,delay,period,on_duration,-1);
}

int DigitalController::addRecursivePwm(uint32_t channels,
  long delay,
  RecursivePwmValues periods,
  RecursivePwmValues on_durations,
  long count)
{
  if (indexed_pwm_.full() || (event_controller_.eventsAvailable() < 2))
  {
    return constants::NO_PWM_AVAILABLE_INDEX;
  }

  size_t level_count = periods.size();
  if (on_durations.size() != level_count)
  {
    return constants::PWM_ARRAY_LENGTHS_NOT_EQUAL_INDEX;
  }

  if (level_count == 0)
  {
    return constants::PWM_ARRAY_LENGTHS_ARE_ZERO_INDEX;
  }

  int pwm_index = constants::NO_CHILD_PWM_INDEX;

  constants::PwmInfo pwm_info;
  for (uint8_t level=0; level < level_count; ++level)
  {
    pwm_info.channels = channels;
    pwm_info.running = false;
    pwm_info.level = level;
    pwm_info.child_index = pwm_index;
    pwm_info.period = periods[level];
    pwm_info.on_duration = on_durations[level];
    if (level == (level_count - 1))
    {
      pwm_info.top_level = true;
      pwm_info.delay = delay;
      pwm_info.count = count;
    }
    else
    {
      pwm_info.top_level = false;
      pwm_info.delay = 0;
      pwm_info.count = -1;
    }
    pwm_info.stopped_before_count_completed = false;
    pwm_info.functor_count_completed = functor_dummy_;
    pwm_info.functor_arg = -1;
    pwm_index = indexed_pwm_.add(pwm_info);
  }

  if (pwm_index != constants::NO_CHILD_PWM_INDEX)
  {
    EventIdPair event_id_pair = event_controller_.addPwmUsingDelay(makeFunctor((Functor1<int> *)0,*this,&DigitalController::startRecursivePwmHandler),
      makeFunctor((Functor1<int> *)0,*this,&DigitalController::stopRecursivePwmHandler),
      delay,
      pwm_info.period,
      pwm_info.on_duration,
      count,
      pwm_index);
    event_controller_.addStartFunctor(event_id_pair,makeFunctor((Functor1<int> *)0,*this,&DigitalController::startPwmHandler));
    event_controller_.addStopFunctor(event_id_pair,makeFunctor((Functor1<int> *)0,*this,&DigitalController::stopPwmHandler));
    indexed_pwm_[pwm_index].event_id_pair = event_id_pair;
    event_controller_.enable(event_id_pair);
  }

  return pwm_index;
}

int DigitalController::startRecursivePwm(uint32_t channels,
  long delay,
  RecursivePwmValues periods,
  RecursivePwmValues on_durations)
{
  return addRecursivePwm(channels,delay,periods,on_durations,-1);
}

void DigitalController::addCountCompletedFunctor(int pwm_index,
  const Functor1<int> & functor,
  int arg)
{
  if (pwm_index < 0)
  {
    return;
  }
  if (indexed_pwm_.indexHasValue(pwm_index))
  {
    constants::PwmInfo & pwm_info = indexed_pwm_[pwm_index];
    pwm_info.functor_count_completed = functor;
    pwm_info.functor_arg = arg;
  }
}

void DigitalController::stopPwm(int pwm_index)
{
  if (pwm_index < 0)
  {
    return;
  }
  if (indexed_pwm_.indexHasValue(pwm_index))
  {
    constants::PwmInfo & pwm_info = indexed_pwm_[pwm_index];
    if (pwm_info.child_index >= 0)
    {
      stopPwm(pwm_info.child_index);
    }
    pwm_info.stopped_before_count_completed = true;
    event_controller_.remove(pwm_info.event_id_pair);
  }
}

void DigitalController::stopAllPwm()
{
  for (size_t i=0; i<constants::INDEXED_PWM_COUNT_MAX; ++i)
  {
    stopPwm(i);
  }
  event_controller_.clearAllEvents();
  indexed_pwm_.clear();
}

void DigitalController::addEventUsingDelay(const Functor1<int> & functor,
  uint32_t delay,
  int arg)
{
  if (event_controller_.eventsAvailable() == 0)
  {
    return;
  }
  EventId event_id = event_controller_.addEventUsingDelay(functor,delay,arg);
  event_controller_.enable(event_id);
}

DigitalController::ChannelsPwmIndexes DigitalController::getChannelsPwmIndexes()
{
  ChannelsPwmIndexes channels_pwm_indexes;
  noInterrupts();
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    RecursivePwmValues channel_pwm_indexes(channels_pwm_indexes_[channel]);
    channels_pwm_indexes.push_back(channel_pwm_indexes);
  }
  interrupts();
  return channels_pwm_indexes;
}

uint32_t DigitalController::arrayToChannels(ArduinoJson::JsonArray & channels_array)
{
  uint32_t channels = 0;
  uint32_t bit = 1;
  for (ArduinoJson::JsonArray::iterator channels_it=channels_array.begin();
       channels_it != channels_array.end();
       ++channels_it)
  {
    long channel = *channels_it;
    channels |= bit << channel;
  }
  return channels;
}

DigitalController::RecursivePwmValues DigitalController::arrayToRecursivePwmValues(ArduinoJson::JsonArray & array)
{
  RecursivePwmValues pwm_values;
  for (ArduinoJson::JsonArray::iterator array_it=array.begin();
       array_it != array.end();
       ++array_it)
  {
    long value = *array_it;
    pwm_values.push_back(value);
  }
  return pwm_values;
}

void DigitalController::removeParentAndChildrenPwmInfo(int pwm_index)
{
  if (pwm_index >= 0)
  {
    if (indexed_pwm_.indexHasValue(pwm_index))
    {
      removeParentAndChildrenPwmInfo(indexed_pwm_[pwm_index].child_index);
      indexed_pwm_.remove(pwm_index);
    }
  }
}

long DigitalController::powerToAnalogWriteValue(long power)
{
  long pwm_value = map(power,
    constants::power_min,
    constants::power_max,
    constants::channel_pwm_min,
    constants::channel_pwm_max);
  long analog_write_value = map(pwm_value,
    constants::channel_pwm_min,
    constants::channel_pwm_max,
    constants::analog_write_min,
    constants::analog_write_max);
  return analog_write_value;
}

void DigitalController::setPowersToMax()
{
  modular_server::Property & power_max_property = modular_server_.property(constants::power_max_property_name);
  long power_max;
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    power_max_property.getElementValue(channel,power_max);
    noInterrupts();
    powers_when_on_[channel] = power_max;
    interrupts();
  }
}

void DigitalController::updateChannel(size_t channel)
{
  uint32_t bit = 1;
  bit = bit << channel;
  noInterrupts();
  uint32_t channels = channels_;
  interrupts();
  if (bit & channels)
  {
    setChannelOn(channel);
  }
}

void DigitalController::updateAllChannels()
{
  noInterrupts();
  uint32_t channels = channels_;
  interrupts();
  setChannels(channels);
}

void DigitalController::initializePwmIndexes()
{
  noInterrupts();
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    for (size_t level=0; level<constants::PWM_LEVEL_COUNT_MAX; ++level)
    {
      channels_pwm_indexes_[channel][level] = constants::PWM_NOT_RUNNING_INDEX;
    }
  }
  interrupts();
}

void DigitalController::setChannelPwmIndexesRunning(size_t channel,
  size_t level,
  int pwm_index)
{
  if ((channel < constants::CHANNEL_COUNT_MAX) && (level < constants::PWM_LEVEL_COUNT_MAX))
  {
    noInterrupts();
    channels_pwm_indexes_[channel][level] = pwm_index;
    interrupts();
  }
}

void DigitalController::setChannelsPwmIndexesRunning(uint32_t channels,
  size_t level,
  int pwm_index)
{
  if (level < constants::PWM_LEVEL_COUNT_MAX)
  {
    uint32_t bit = 1;
    noInterrupts();
    for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
    {
      if ((bit << channel) & channels)
      {
        channels_pwm_indexes_[channel][level] = pwm_index;
      }
    }
    interrupts();
  }
}

void DigitalController::setChannelPwmIndexesStopped(size_t channel,
  size_t level)
{
  if ((channel < constants::CHANNEL_COUNT_MAX) && (level < constants::PWM_LEVEL_COUNT_MAX))
  {
    noInterrupts();
    channels_pwm_indexes_[channel][level] = constants::PWM_NOT_RUNNING_INDEX;
    interrupts();
  }
}

void DigitalController::setChannelsPwmIndexesStopped(uint32_t channels,
  size_t level)
{
  if (level < constants::PWM_LEVEL_COUNT_MAX)
  {
    uint32_t bit = 1;
    noInterrupts();
    for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
    {
      if ((bit << channel) & channels)
      {
        channels_pwm_indexes_[channel][level] = constants::PWM_NOT_RUNNING_INDEX;
      }
    }
    interrupts();
  }
}

void DigitalController::returnPwmIndexResponse(int pwm_index)
{
  if (pwm_index >= 0)
  {
    modular_server_.response().returnResult(pwm_index);
  }
  else if (pwm_index == constants::NO_PWM_AVAILABLE_INDEX)
  {
    modular_server_.response().returnError(constants::no_pwm_available_error);
  }
  else if (pwm_index == constants::PWM_ARRAY_LENGTHS_NOT_EQUAL_INDEX)
  {
    modular_server_.response().returnError(constants::pwm_array_lengths_not_equal_error);
  }
  else if (pwm_index == constants::PWM_ARRAY_LENGTHS_ARE_ZERO_INDEX)
  {
    modular_server_.response().returnError(constants::pwm_array_lengths_are_zero_error);
  }
}

// Handlers must be non-blocking (avoid 'delay')
//
// modular_server_.parameter(parameter_name).getValue(value) value type must be either:
// fixed-point number (int, long, etc.)
// floating-point number (float, double)
// bool
// const char *
// ArduinoJson::JsonArray *
// ArduinoJson::JsonObject *
// const ConstantString *
//
// For more info read about ArduinoJson parsing https://github.com/janelia-arduino/ArduinoJson
//
// modular_server_.property(property_name).getValue(value) value type must match the property default type
// modular_server_.property(property_name).setValue(value) value type must match the property default type
// modular_server_.property(property_name).getElementValue(element_index,value) value type must match the property array element default type
// modular_server_.property(property_name).setElementValue(element_index,value) value type must match the property array element default type

void DigitalController::startPwmHandler(int pwm_index)
{
  uint32_t & channels = indexed_pwm_[pwm_index].channels;
  uint8_t & level = indexed_pwm_[pwm_index].level;

  setChannelsPwmIndexesRunning(channels,level,pwm_index);
  indexed_pwm_[pwm_index].running = true;
}

void DigitalController::stopPwmHandler(int pwm_index)
{
  constants::PwmInfo pwm_info = indexed_pwm_[pwm_index];
  uint32_t & channels = pwm_info.channels;
  uint8_t & level = pwm_info.level;
  bool stopped_before_count_completed = pwm_info.stopped_before_count_completed;
  Functor1<int> functor_count_completed = pwm_info.functor_count_completed;
  int functor_arg = pwm_info.functor_arg;
  if (level == 0)
  {
    setChannelsOff(channels);
  }
  setChannelsPwmIndexesStopped(channels,level);
  indexed_pwm_[pwm_index].running = false;
  if (pwm_info.top_level)
  {
    removeParentAndChildrenPwmInfo(pwm_index);
  }
  if (!stopped_before_count_completed && functor_count_completed)
  {
    functor_count_completed(functor_arg);
  }
}

void DigitalController::setChannelCountHandler()
{
  long channel_count = getChannelCount();
  long channel_max = channel_count - 1;

  modular_server::Property & power_max_property = modular_server_.property(constants::power_max_property_name);
  power_max_property.setArrayLengthRange(channel_count,channel_count);

  modular_server::Parameter & channel_parameter = modular_server_.parameter(constants::channel_parameter_name);
  channel_parameter.setRange(constants::channel_min,channel_max);

  modular_server::Parameter & channels_parameter = modular_server_.parameter(constants::channels_parameter_name);
  channels_parameter.setRange(constants::channel_min,channel_max);
  channels_parameter.setArrayLengthRange(constants::channel_count_min,channel_count);

  modular_server::Parameter & powers_parameter = modular_server_.parameter(constants::powers_parameter_name);
  powers_parameter.setArrayLengthRange(channel_count,channel_count);

}

void DigitalController::setPowerMaxHandler(size_t channel)
{
  modular_server::Property & power_max_property = modular_server_.property(constants::power_max_property_name);
  long power_max;
  power_max_property.getElementValue(channel,power_max);
  noInterrupts();
  if (powers_when_on_[channel] > power_max)
  {
    powers_when_on_[channel] = power_max;
  }
  interrupts();

  updateChannel(channel);
}

void DigitalController::enableAllHandler()
{
  enableAll();
}

void DigitalController::disableAllHandler()
{
  disableAll();
}

void DigitalController::enabledHandler()
{
  bool all_enabled = enabled();
  modular_server_.response().returnResult(all_enabled);
}

void DigitalController::setPowerWhenOnHandler()
{
  size_t channel;
  modular_server_.parameter(constants::channel_parameter_name).getValue(channel);
  size_t power;
  modular_server_.parameter(constants::power_parameter_name).getValue(power);
  power = setPowerWhenOn(channel,power);
  modular_server_.response().returnResult(power);
}

void DigitalController::setPowersWhenOnHandler()
{
  ArduinoJson::JsonArray * powers_array_ptr;
  modular_server_.parameter(constants::powers_parameter_name).getValue(powers_array_ptr);

  modular_server_.response().writeResultKey();
  modular_server_.response().beginArray();

  size_t channel = 0;
  for (ArduinoJson::JsonArray::iterator powers_it=powers_array_ptr->begin();
       powers_it != powers_array_ptr->end();
       ++powers_it)
  {
    long power = *powers_it;
    power = setPowerWhenOn(channel,power);
    modular_server_.response().write(power);
    ++channel;
  }

  modular_server_.response().endArray();
}

void DigitalController::setAllPowersWhenOnHandler()
{
  size_t power_to_set;
  modular_server_.parameter(constants::power_parameter_name).getValue(power_to_set);

  modular_server_.response().writeResultKey();
  modular_server_.response().beginArray();

  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    long power = setPowerWhenOn(channel,power_to_set);
    modular_server_.response().write(power);
  }

  modular_server_.response().endArray();
}

void DigitalController::getPowersWhenOnHandler()
{
  modular_server_.response().writeResultKey();
  modular_server_.response().beginArray();
  long power;
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    power = getPowerWhenOn(channel);
    modular_server_.response().write(power);
  }
  modular_server_.response().endArray();
}

void DigitalController::getPowersHandler()
{
  modular_server_.response().writeResultKey();
  modular_server_.response().beginArray();
  long power;
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    power = getPower(channel);
    modular_server_.response().write(power);
  }
  modular_server_.response().endArray();
}

void DigitalController::setChannelOnHandler()
{
  size_t channel;
  modular_server_.parameter(constants::channel_parameter_name).getValue(channel);
  setChannelOn(channel);
}

void DigitalController::setChannelOnAtPowerHandler()
{
  size_t channel;
  modular_server_.parameter(constants::channel_parameter_name).getValue(channel);
  size_t power;
  modular_server_.parameter(constants::power_parameter_name).getValue(power);
  setChannelOnAtPower(channel,power);
}

void DigitalController::setChannelOffHandler()
{
  size_t channel;
  modular_server_.parameter(constants::channel_parameter_name).getValue(channel);
  setChannelOff(channel);
}

void DigitalController::setChannelsOnHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  setChannelsOn(channels);
}

void DigitalController::setChannelsOnAtPowerHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  size_t power;
  modular_server_.parameter(constants::power_parameter_name).getValue(power);
  setChannelsOnAtPower(channels,power);
}

void DigitalController::setChannelsOffHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  setChannelsOff(channels);
}

void DigitalController::toggleChannelHandler()
{
  size_t channel;
  modular_server_.parameter(constants::channel_parameter_name).getValue(channel);
  toggleChannel(channel);
}

void DigitalController::toggleChannelsHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  toggleChannels(channels);
}

void DigitalController::toggleAllChannelsHandler()
{
  toggleAllChannels();
}

void DigitalController::setAllChannelsOnHandler()
{
  setAllChannelsOn();
}

void DigitalController::setAllChannelsOffHandler()
{
  setAllChannelsOff();
}

void DigitalController::setChannelOnAllOthersOffHandler()
{
  size_t channel;
  modular_server_.parameter(constants::channel_parameter_name).getValue(channel);
  setChannelOnAllOthersOff(channel);
}

void DigitalController::setChannelOffAllOthersOnHandler()
{
  size_t channel;
  modular_server_.parameter(constants::channel_parameter_name).getValue(channel);
  setChannelOffAllOthersOn(channel);
}

void DigitalController::setChannelsOnAllOthersOffHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  setChannelsOnAllOthersOff(channels);
}

void DigitalController::setChannelsOffAllOthersOnHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  setChannelsOffAllOthersOn(channels);
}

void DigitalController::channelIsOnHandler()
{
  size_t channel;
  modular_server_.parameter(constants::channel_parameter_name).getValue(channel);
  bool channel_is_on = channelIsOn(channel);
  modular_server_.response().returnResult(channel_is_on);
}

void DigitalController::getChannelsOnHandler()
{
  uint32_t channels_on = getChannelsOn();
  uint32_t bit = 1;
  modular_server_.response().writeResultKey();
  modular_server_.response().beginArray();
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    if (channels_on & (bit << channel))
    {
      modular_server_.response().write(channel);
    }
  }
  modular_server_.response().endArray();
}

void DigitalController::getChannelsOffHandler()
{
  uint32_t channels_on = getChannelsOn();
  uint32_t channels_off = ~channels_on;
  uint32_t bit = 1;
  modular_server_.response().writeResultKey();
  modular_server_.response().beginArray();
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    if (channels_off & (bit << channel))
    {
      modular_server_.response().write(channel);
    }
  }
  modular_server_.response().endArray();
}

void DigitalController::getChannelCountHandler()
{
  size_t channel_count = getChannelCount();
  modular_server_.response().returnResult(channel_count);
}

void DigitalController::addPwmHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  long delay;
  modular_server_.parameter(constants::delay_parameter_name).getValue(delay);
  long period;
  modular_server_.parameter(constants::period_parameter_name).getValue(period);
  long on_duration;
  modular_server_.parameter(constants::on_duration_parameter_name).getValue(on_duration);
  long count;
  modular_server_.parameter(constants::count_parameter_name).getValue(count);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  int pwm_index = addPwm(channels,delay,period,on_duration,count);
  returnPwmIndexResponse(pwm_index);
}

void DigitalController::startPwmHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  long delay;
  modular_server_.parameter(constants::delay_parameter_name).getValue(delay);
  long period;
  modular_server_.parameter(constants::period_parameter_name).getValue(period);
  long on_duration;
  modular_server_.parameter(constants::on_duration_parameter_name).getValue(on_duration);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  int pwm_index = startPwm(channels,delay,period,on_duration);
  returnPwmIndexResponse(pwm_index);
}

void DigitalController::addRecursivePwmHandler()
{
  ArduinoJson::JsonArray * channels_array_ptr;
  modular_server_.parameter(constants::channels_parameter_name).getValue(channels_array_ptr);
  long delay;
  modular_server_.parameter(constants::delay_parameter_name).getValue(delay);
  ArduinoJson::JsonArray * periods_array_ptr;
  modular_server_.parameter(constants::periods_parameter_name).getValue(periods_array_ptr);
  ArduinoJson::JsonArray * on_durations_array_ptr;
  modular_server_.parameter(constants::on_durations_parameter_name).getValue(on_durations_array_ptr);
  long count;
  modular_server_.parameter(constants::count_parameter_name).getValue(count);
  const uint32_t channels = arrayToChannels(*channels_array_ptr);
  RecursivePwmValues periods = arrayToRecursivePwmValues(*periods_array_ptr);
  RecursivePwmValues on_durations = arrayToRecursivePwmValues(*on_durations_array_ptr);
  int pwm_index = addRecursivePwm(channels,delay,periods,on_durations,count);
  returnPwmIndexResponse(pwm_index);
}

void DigitalController::startRecursivePwmHandler()
{
}

void DigitalController::stopPwmHandler()
{
  int pwm_index;
  modular_server_.parameter(constants::pwm_index_parameter_name).getValue(pwm_index);
  stopPwm(pwm_index);
}

void DigitalController::stopAllPwmHandler()
{
  stopAllPwm();
}

void DigitalController::getChannelsPwmIndexesHandler()
{
  modular_server_.response().writeResultKey();
  modular_server_.response().beginArray();

  ChannelsPwmIndexes channels_pwm_indexes = getChannelsPwmIndexes();
  for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
  {
    modular_server_.response().beginArray();

    for (size_t level=0; level<constants::PWM_LEVEL_COUNT_MAX; ++level)
    {
      modular_server_.response().write(channels_pwm_indexes[channel][level]);
    }

    modular_server_.response().endArray();
  }

  modular_server_.response().endArray();

}

void DigitalController::getPwmInfoHandler()
{
  noInterrupts();
  IndexedContainer<constants::PwmInfo,
    constants::INDEXED_PWM_COUNT_MAX> indexed_pwm = indexed_pwm_;
  interrupts();

  uint32_t bit = 1;

  modular_server_.response().writeResultKey();
  modular_server_.response().beginArray();

  for (size_t i=0; i<constants::INDEXED_PWM_COUNT_MAX; ++i)
  {
    if (indexed_pwm.indexHasValue(i))
    {
      modular_server_.response().beginObject();
      constants::PwmInfo & pwm_info = indexed_pwm[i];
      modular_server_.response().write(constants::pwm_index_parameter_name,i);
      modular_server_.response().writeKey(constants::channels_parameter_name);
      modular_server_.response().beginArray();
      for (size_t channel=0; channel<constants::CHANNEL_COUNT_MAX; ++channel)
      {
        if ((bit << channel) & pwm_info.channels)
        {
          modular_server_.response().write(channel);
        }
      }
      modular_server_.response().endArray();
      modular_server_.response().write(constants::running_string,pwm_info.running);
      modular_server_.response().write(constants::level_string,pwm_info.level);
      modular_server_.response().write(constants::top_level_string,pwm_info.top_level);
      modular_server_.response().write(constants::child_index_string,pwm_info.child_index);
      modular_server_.response().write(constants::delay_parameter_name,pwm_info.delay);
      modular_server_.response().write(constants::period_parameter_name,pwm_info.period);
      modular_server_.response().write(constants::on_duration_parameter_name,pwm_info.on_duration);
      modular_server_.response().write(constants::count_parameter_name,pwm_info.count);
      modular_server_.response().endObject();
    }
  }

  modular_server_.response().endArray();

}

void DigitalController::setChannelsOnHandler(int pwm_index)
{
  uint32_t & channels = indexed_pwm_[pwm_index].channels;
  setChannelsOn(channels);
}

void DigitalController::setChannelsOffHandler(int pwm_index)
{
  uint32_t & channels = indexed_pwm_[pwm_index].channels;
  setChannelsOff(channels);
}

void DigitalController::startRecursivePwmHandler(int pwm_index)
{
  constants::PwmInfo pwm_info = indexed_pwm_[pwm_index];
  int child_index = pwm_info.child_index;
  if (pwm_info.level == 0)
  {
    uint32_t channels = pwm_info.channels;
    setChannelsOn(channels);
  }
  else
  {
    long delay = indexed_pwm_[child_index].delay;
    long period = indexed_pwm_[child_index].period;
    long on_duration = indexed_pwm_[child_index].on_duration;
    EventIdPair event_id_pair = event_controller_.addInfinitePwmUsingDelay(makeFunctor((Functor1<int> *)0,*this,&DigitalController::startRecursivePwmHandler),
      makeFunctor((Functor1<int> *)0,*this,&DigitalController::stopRecursivePwmHandler),
      delay,
      period,
      on_duration,
      child_index);
    event_controller_.addStartFunctor(event_id_pair,makeFunctor((Functor1<int> *)0,*this,&DigitalController::startPwmHandler));
    event_controller_.addStopFunctor(event_id_pair,makeFunctor((Functor1<int> *)0,*this,&DigitalController::stopPwmHandler));
    indexed_pwm_[child_index].event_id_pair = event_id_pair;
    event_controller_.enable(event_id_pair);
  }
}

void DigitalController::stopRecursivePwmHandler(int pwm_index)
{
  constants::PwmInfo pwm_info = indexed_pwm_[pwm_index];
  if (pwm_info.level == 0)
  {
    uint32_t channels = pwm_info.channels;
    setChannelsOff(channels);
  }
  else
  {
    int child_index = pwm_info.child_index;
    if (child_index >= 0)
    {
      event_controller_.remove(indexed_pwm_[child_index].event_id_pair);
      stopRecursivePwmHandler(child_index);
    }
  }
}
