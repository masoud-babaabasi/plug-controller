class my_time{
  public:
  uint8_t hour,min,sec;
  my_time(uint8_t _hour=0 , uint8_t _min=0 , uint8_t _sec=0){
    hour = _hour;
    min = _min;
    sec = _sec;
  }
  bool operator == (const my_time t1){
      if( hour != t1.hour) return false;
      if( min != t1.min) return false;
      if( sec != t1.sec) return false;
      return true;
  }  
};
enum days_of_week{
  sat = 0,
  sun,
  mon,
  tue,
  wed,
  thr,
  fri
};

class my_date{
  public:
  uint8_t year;
  uint8_t month , day , DOW ,repeat_days;
  my_date(uint8_t _year = 0 , uint8_t _month = 0 , uint8_t _day = 0 , uint8_t _DOW = 0, uint8_t _repeat_days = 0){
    year = _year;
    month = _month;
    day = _day;
    repeat_days = _repeat_days;
    DOW = _DOW;
  }
  bool operator ==( const my_date d1){
    if( year != d1.year) return false;
    if( month != d1.month) return false;
    if( day != d1.day) return false;
    return true;
  }
  bool have_alaram_on(uint8_t day){
    if( (repeat_days  & (1 << day) ) ) return true;
    return false;
  }
};

class schedule{
  public:
  my_time time;
  my_date date;
  uint8_t action;
  uint8_t active;

  my_date(uint8_t _action = 0 , uitn8_t _active = 0){
    action = _action;
    active = _active;
  }
};