//PAN

#include <iostream>
#include <sstream>
#include <cstdarg>
#include <algorithm>

#ifdef WIN32
#include <sys/stat.h>
#include <Windows.h>
#include <Rpc.h>
#endif

#include "util.h"

//---------------------------------------------------------------------------

// used by FormatAddress to make addresses pretty, concise, but mostly consistent (original list came from USPS website)
const char *hardcoded_usps_abbrs[] = 
{
   // basic/common
   "Po","PO","Ne","NE","Nw","NW","Se","SE","Sw","SW","North","N","South","S","East","E","West","W","Northeast","NE","Northwest","NW","Southeast","SE","Southwest","SW",
   // military
   "Aa","AA","Ae","AE","Ap","AP",
   // primary unit designators
   "Alley","Aly","Annex","Anx","Arcade","Arc","Avenue","Ave","Bayoo","Byu","Bayou","Byu","Beach","Bch","Bend","Bnd","Bluff","Blf","Bluffs","Blfs",
   "Bottom","Btm","Boulevard","Blvd","Branch","Br","Bridge","Brg","Brook","Brk","Brooks","Brks","Burg","Bg","Burgs","Bgs","Bypass","Byp","Camp","Cp",
   "Canyon","Cyn","Cape","Cpe","Causeway","Cswy","Center","Ctr","Centers","Ctrs","Circle","Cir","Circles","Cirs","Cliff","Clf","Cliffs","Clfs","Club","Clb",
   "Common","Cmn","Corner","Cor","Corners","Cors","Course","Crse","Court","Ct","Courts","Cts","Cove","Cv","Coves","Cvs","Creek","Crk","Crescent","Cres",
   "Crest","Crst","Crossing","Xing","Crossroad","Xrd","Curve","Curv","Dale","Dl","Dam","Dm","Divide","Dv","Drive","Dr","Drives","Drs","Estate","Est",
   "Estates","Ests","Expressway","Expy","Extension","Ext","Extention","Ext","Extensions","Exts","Extentions","Exts","Falls","Fls","Ferry","Fry","Field","Fld",
   "Fields","Flds","Flat","Flt","Flats","Flts","Ford","Frd","Fords","Frds","Forest","Frst","Forge","Frg","Forges","Frgs","Fork","Frk","Forks","Frks",
   "Fort","Ft","Freeway","Fwy","Garden","Gdn","Gardens","Gdns","Gateway","Gtwy","Glen","Gln","Glens","Glns","Green","Grn","Greens","Grns","Grove","Grv",
   "Groves","Grvs","Harbor","Hbr","Harbors","Hbrs","Haven","Hvn","Heights","Hts","Highway","Hwy","Hill","Hl","Hills","Hls","Hollow","Holw","Inlet","Inlt",
   "Island","Is","Islands","Iss","Isles","Isle","Junction","Jct","Junctions","Jcts","Key","Ky","Keys","Kys","Knoll","Knl","Knolls","Knls","Lake","Lk",
   "Lakes","Lks","Landing","Lndg","Lane","Ln","Light","Lgt","Lights","Lgts","Loaf","Lf","Lock","Lck","Locks","Lcks","Lodge","Ldg","Loops","Loop",
   "Manor","Mnr","Manors","Mnrs","Meadow","Mdw","Meadows","Mdws","Mill","Ml","Mills","Mls","Mission","Msn","Motorway","Mtwy","Mount","Mt","Mountain","Mtn",
   "Mountains","Mtns","Neck","Nck","Orchard","Orch","Overpass","Opas","Parks","Park","Parkway","Pkwy","Parkways","Pkwy","Passage","Psge","Paths","Path",
   "Pine","Pne","Pines","Pnes","Place","Pl","Plain","Pln","Plains","Plns","Plaza","Plz","Point","Pt","Points","Pts","Port","Prt","Ports","Prts","Prairie","Pr",
   "Radial","Radl","Ranch","Rnch","Ranches","Rnch","Rapid","Rpd","Rapids","Rpds","Rest","Rst","Ridge","Rdg","Ridges","Rdgs","River","Riv","Road","Rd",
   "Roads","Rds","Shoal","Shl","Shoals","Shls","Shore","Shr","Shores","Shrs","Skyway","Skwy","Spring","Spg","Springs","Spgs","Square","Sq","Squares","Sqs",
   "Station","Sta","Stravenue","Stra","Stream","Strm","Street","St","Streets","Sts","Summit","Smt","Terrace","Ter","Throughway","Trwy","Trace","Trce",
   "Track","Trak","Tracks","Trak","Trafficway","Trfy","Trail","Trl","Tunnel","Tunl","Turnpike","Tpke","Underpass","Upas",/*"Union","Un",*/"Unions","Uns",
   // if the FormatAddress() code is updated to identify the parts of an address, then the above commented out abbreviations can be restored.  the problem
   // is that we end up with things like 123 S Un St, or 345 NW Blvd Ave, and that can be confusing
   "Valley","Vly","Valleys","Vlys","Viaduct","Via","View","Vw","Views","Vws","Village","Vlg","Villages","Vlgs","Ville","Vl","Vista","Vis","Walks","Walk",
   "Well","Wl","Wells","Wls",
    // secondary unit designator
   "Apartment","Apt","Basement","Bsmt","Department","Dept","Floor","Fl","Front","Frnt","Hangar","Hngr","Lobby","Lbby","Lower","Lowr","Office","Ofc",
   "Penthouse","Ph","Room","Rm","Space","Spc","Suite","Ste","Trailer","Trlr","Upper","Uppr",
   NULL
};

utilmapstrs usps_abbr_map;

void InitUspsAbbrMap() // initialize the USPS abbreviation map, if not already done
{
   if(usps_abbr_map.empty())
   {
      unsigned int i = 0;
      while(hardcoded_usps_abbrs[i])
      {
         usps_abbr_map.insert(std::pair<utilstr,utilstr>(hardcoded_usps_abbrs[i], hardcoded_usps_abbrs[i+1]));
         i+=2; // pairs
      }
   }
}

// START DATE MANIPULATION STUFF---------------------------------------------

// jhaley 20121120: Alternative implementation of Delphi FileExists, Date(), Time(), and Now() functions.
#if defined(WIN32) && !defined(__BORLANDC__)

utilstr Date()
{
   char buf[16];
   SYSTEMTIME st;

   memset(buf, 0, sizeof(buf));

   GetLocalTime(&st);
   snprintf(buf, sizeof(buf), "%d/%d/%d", st.wMonth, st.wDay, st.wYear);

   return utilstr(buf);
}

utilstr Time()
{
   char buf[16];
   SYSTEMTIME st;

   memset(buf, 0, sizeof(buf));

   GetLocalTime(&st);
   snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", 
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

   return utilstr(buf);
}

utilstr Now()
{
   char buf[32];
   SYSTEMTIME st;

   memset(buf, 0, sizeof(buf));

   GetLocalTime(&st);
   snprintf(buf, sizeof(buf), "%d/%d/%d %02d:%02d:%02d.%03d",
            st.wMonth, st.wDay, st.wYear, 
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

   return utilstr(buf);
}

bool FileExists(const char *filename)
{
   struct stat sbuf;

   if(!stat(filename, &sbuf)) // Exists?
      return !(sbuf.st_mode & S_IFDIR); // check not a directory.

   return false;
}

#endif

static int daysinnorm[13] = 
{ 
   0,
   31, 28, 31, 30,
   31, 30, 31, 31,
   30, 31, 30, 31
};

static int daysinleap[13] = 
{ 
   0,
   31, 29, 31, 30,
   31, 30, 31, 31,
   30, 31, 30, 31
};

static int month_code_norm[13] = { 0, 1, 4, 4, 0, 2, 5, 0, 3, 6, 1, 4, 6 };
static int month_code_leap[13] = { 0, 0, 3, 4, 0, 2, 5, 0, 3, 6, 1, 4, 6 };

bool IsLeapYear(int year)
{
   if(year % 4)    return false; // not divisible by 4, not leap
   if(year < 1582) return true;  // divisible by 4 before 1582, leap
   if(year % 100)  return true;  // divisible by 4, not by 100, leap
   if(year % 400)  return false; // divisible by 4, 100, not 400, not leap
   return true;                  // divisible by 400, leap
}

int LastDayOf(int month, int year)
{
   return (IsLeapYear(year) ? daysinleap[month] : daysinnorm[month]);
}

//pad to the left with a zero to make it two digits.
utilstr twodigit(int incoming)
{
   utilstr answer = IntToString(incoming);
   if(answer.length() < 2) 
      answer = "0" + answer;
   return answer;
}

// jhaley 20120914: likewise, but for milliseconds - three digits are needed
utilstr threedigit(int incoming)
{
   utilstr answer = IntToString(incoming);
   if(answer.length() < 3)
   {
     do { answer = "0" + answer; } while(answer.length() != 3);
   }
   return answer;
}

// separate a string into three integers, ignoring leading spaces,
// and differentiating the three integers by a single character.
// used both for time and date parsing, to get h:m:s or m/d/y
// or any other combination (see the date stuff to understand.)
void septhree(utilcstr incoming, char separator, int &A, int&B, int&C)
{
   unsigned int beginA; // first digit
   unsigned int beginB; // first mark
   unsigned int beginC; // second mark
   unsigned int beginD; // after last digit

   beginA = incoming.find_first_of("0123456789");

   if(beginA < incoming.length() - 1 && beginA != utilstr::npos)
   {
      beginB = incoming.find_first_of (separator, beginA + 1);
      if(beginB < incoming.length() - 1 && beginB != utilstr::npos)
      {
         A = StringToInt(incoming.substr(beginA, beginB - beginA));
         beginC = incoming.find_first_of (separator, beginB + 1);
         if(beginC < incoming.length() - 1 && beginC != utilstr::npos)
         {
            B = StringToInt(incoming.substr(beginB + 1, beginC - beginB - 1));
            beginD = incoming.find_first_not_of ("0123456789", beginC + 1);
            if(beginD == utilstr::npos)
               C = StringToInt(incoming.substr(beginC + 1));
            else
               C = StringToInt(incoming.substr(beginC + 1, beginD - beginC - 1));
         }
         else if(beginC == utilstr::npos)
            B = StringToInt(incoming.substr(beginB + 1));
      }
      else if(beginB == utilstr::npos)
         A = StringToInt(incoming.substr(beginA));
   }
}

// END DATE MANIPULATION STUFF-----------------------------------------------

// START DATE TIME TIMESTAMP CLASS DEFINITIONS-------------------------------
// Pstamp is derived from Pdate and Ptime. As such, it is mostly a combination
// of code from its ancestors. Not much is added. All three support the
// .Out* (mode), .In* (string, mode), and .IsValid* () functions, where * is
// one of time, date, or stamp. They all support empty constructors and
// constructors with (string, mode) passed.
// Note on "mode" - this value indicates how the string is to be interpreted.
// It's okay to pass time values to date, or date to time, because the bits
// don't overlap. In the case of stamp, you'll want to pass some of each,
// which will be passed on to the base class constructors. There are defaults.
// To see what the defaults are, check the .h file and see which values have
// a zero next to them.
// Internally, everything is kept in 4-digit year, military time format.
// All three classes support the <, >, <=, >=, ==, != relational operators.

// DATE----------------------------------------------------------------------

Pdate::Pdate()
{
   year = 0;
   month = 0;
   day = 0;
}

Pdate::Pdate(utilcstr incoming, int mode)
{
   InDate(incoming, mode);
}

//
// Pdate Constructor
//
// jhaley 20120907: Takes a standard C time_t value.
//
Pdate::Pdate(const time_t *time)
{
   InDate(time);
}

void Pdate::InDate(const time_t *time)
{
   struct tm *lt = localtime(time);

   year  = lt->tm_year + 1900;
   month = lt->tm_mon  + 1;
   day   = lt->tm_mday;
}

void Pdate::InDate(utilcstr incoming, int mode)
{
   int y;
   int m;
   int d;

   int a = 0;
   int b = 0;
   int c = 0;

   // jhaley 20151009: allow '-' in dates as well as '/'
   char sepchar = datechar;
   if(!incoming.empty() && incoming.find(sepchar) == utilstr::npos)
      sepchar = datecharalt;

   // take three chunks out
   septhree (incoming, sepchar, a, b, c);

   // determine which is which
   if(mode & ymd)
   {
      y = a;
      m = b;
      d = c;
   }
   else if(mode & dmy)
   {
      d = a;
      m = b;
      y = c;
   }
   else //mdy
   {
      m = a;
      d = b;
      y = c;
   }

   //convert two-digit year to four-digit year
   if (!(mode & yyyy) && y < 200)
   {
      y += 1900;
      if(y < y2kcut) 
         y += 100;
   }

   //store locally
   year = y;
   month = m;
   day = d;
}

void Pdate::InDate (const Pdate & incoming)
{
   year  = incoming.year;
   month = incoming.month;
   day   = incoming.day;
}

bool Pdate::IsValidDate () const
{
   if(month < 1 || month > 12) 
      return false;
   if(year < 1) 
      return false;
   if(day < 1) 
      return false;
   if(day > LastDayOf (month, year)) 
      return false;
   return true;
}

utilstr Pdate::OutDate (int mode) const
{
   utilstr y, m, d, c;

   y = IntToString(year);
   
   if(mode & left_zeros)
      m = twodigit(month);
   else
      m = IntToString(month);
   
   if(mode & left_zeros)
      d = twodigit(day);
   else
      d = IntToString(day);
   
   c = "";
   
   if(!(mode & no_separators)) // jhaley 20120723
      c += datechar;
   
   utilstr answer = "";
   
   if(!(mode & yyyy))
      y = y.substr(2);

   if(mode & ymd)
      answer = y + c + m + c + d;
   else if(mode & dmy)
      answer = d + c + m + c + y;
   else // (mode & mdy)
      answer = m + c + d + c + y;
   
   return answer;
}

bool Pdate::operator < (const Pdate & rhs) const
{
   if(year < rhs.year) return true;
   if(year > rhs.year) return false;
 
   // same year
   if(month < rhs.month) return true;
   if(month > rhs.month) return false;
 
   // same month
   if(day < rhs.day) return true;
 
   // doesn't matter anymore
   return false;
}

bool Pdate::operator <= (const Pdate & rhs) const
{
   if(year < rhs.year) return true;
   if(year > rhs.year) return false;
 
   // same year
   if(month < rhs.month) return true;
   if(month > rhs.month) return false;
 
   // same month
   if(day <= rhs.day) return true;
 
   return false;
}

bool Pdate::operator > (const Pdate & rhs) const
{
   if(year > rhs.year) return true;
   if(year < rhs.year) return false;
 
   //same year
   if(month > rhs.month) return true;
   if(month < rhs.month) return false;
 
   //same month
   if(day > rhs.day) return true;
 
   //doesn't matter anymore
   return false;
}

bool Pdate::operator >= (const Pdate & rhs) const
{
   if (year > rhs.year) return true;
   if (year < rhs.year) return false;
 
   //same year
   if (month > rhs.month) return true;
   if (month < rhs.month) return false;
 
   //same month
   if (day >= rhs.day) return true;
 
   //doesn't matter anymore
   return false;
}

bool Pdate::operator != (const Pdate & rhs) const
{
   return (year != rhs.year || month != rhs.month || day != rhs.day);
}

bool Pdate::operator == (const Pdate & rhs) const
{
   return (year == rhs.year && month == rhs.month && day == rhs.day);
}

Pdate Pdate::operator ++ (int)
{
   day++;
   int upper_bound = LastDayOf (month, year);
   if(day > upper_bound)
   {
      month++;
      if(month > 12)
      {
         year++;
         month = 1;
      }
      day = 1;
   }
   return *this;
}

Pdate Pdate::operator -- (int)
{
   day--;
   if(day < 1)
   {
      month--;
      if(month < 1)
      {
         year--;
         month = 12;
      }
      int upper_bound = LastDayOf (month, year);
      day = upper_bound;
   }
   return *this;
}

int Pdate::mod (int p_distance, int p_metric)
{
   if(p_metric == in_days)
   {
      while (p_distance)
      {
         if (p_distance > 0)
         {
            (*this)++;
            p_distance--;
         }
         else
         {
            (*this)--;
            p_distance++;
         }
      }
   }
   else if (p_metric == in_weeks)
   {
      while (p_distance)
      {
         if (p_distance > 0)
         {
            for (int i = 0; i < 7; i++) (*this)++;
            p_distance--;
         }
         else
         {
            for (int i = 0; i < 7; i++) (*this)--;
            p_distance++;
         }
      }
   }
   else if (p_metric == in_months)
   {
      while (p_distance)
      {
         if (p_distance > 0)
         {
            month++;
            if (month > 12)
            {
               month = 1;
               year++;
            }
            p_distance--;
         }
         else
         {
            month--;
            if (month < 1)
            {
               month = 12;
               year--;
            }
            p_distance++;
         }
      }
   }
   else if (p_metric == in_years)
   {
      while (p_distance)
      {
         if (p_distance > 0)
         {
            year++;
            p_distance--;
         }
         else
         {
            year--;
            p_distance++;
         }
      }
   }

   // handle both leap years and changing from a month with 31 days
   // to a month with 30 days, but only after done moving through the
   // months, etc. so as not to have to go up and down, etc.
   // oh, and much faster!
   if(day > LastDayOf(month, year))
      day = LastDayOf(month, year);
   
   return 0;
}

int Pdate::mod (const Pdate & p_distance)
{
   this->mod(p_distance.year,  in_years);
   this->mod(p_distance.month, in_months);
   this->mod(p_distance.day,   in_days);
   return 0;
}

int Pdate::DayOfWeek () const
{
   int c, y, m, d;
   int C, Y, M, D;
   int x;

   // initial values;
   c = (year / 100) + 1;
   y = year - ((c - 1) * 100);
   m = month;
   d = day;

   // codes
   C = (8 - ((c % 4) * 2)) % 8;
   Y = (y + (y / 4)) % 7;
   M = (IsLeapYear(year)) ? month_code_leap[m] : month_code_norm[m];
   D = d;

   // day of week
   x = (C + Y + M + D) % 7;

   // so far, sunday is 1, friday is 6, saturday is 0
   // since ashley's code has sunday as 1 and saturday as 7,...
   if(!x) 
      x = 7;

   return x;
}

utilstr Pdate::NameOfDay() const
{
   switch(DayOfWeek())
   {
   case 1:  return "Sunday";
   case 2:  return "Monday";
   case 3:  return "Tuesday";
   case 4:  return "Wednesday";
   case 5:  return "Thursday";
   case 6:  return "Friday";
   case 7:  return "Saturday";
   default: return "";
   }
}

utilstr Pdate::NameOfMonth() const
{
   switch(month)
   {
   case 1:  return "January";
   case 2:  return "February";
   case 3:  return "March";
   case 4:  return "April";
   case 5:  return "May";
   case 6:  return "June";
   case 7:  return "July";
   case 8:  return "August";
   case 9:  return "September";
   case 10: return "October";
   case 11: return "November";
   case 12: return "December";
   default: return "";
   }
}

Pdate Pdate::operator += (int rhs)
{
   if(rhs > 0)
   {
      while(rhs > 0)
      {
         (*this)++;
         rhs--;
      }
   }
   else if(rhs < 0)
   {
      while(rhs < 0)
      {
         (*this)--;
         rhs++;
      }
   }
   return *this;
}

Pdate Pdate::operator -= (int rhs)
{
   if(rhs > 0)
   {
      while(rhs > 0)
      {
         (*this)--;
         rhs--;
      }
   }
   else if(rhs < 0)
   {
      while(rhs < 0)
      {
         (*this)++;
         rhs++;
      }
   }
   return *this;
}

// external operators (don't need "friend", but it would make sense)

Pdate operator + (const Pdate & lhs, const int & rhs)
{
   Pdate answer = lhs;
   answer += rhs;
   return answer;
}

Pdate operator - (const Pdate & lhs, const int & rhs)
{
   Pdate answer = lhs;
   answer -= rhs;
   return answer;
}

int operator - (const Pdate & lhs, const Pdate & rhs)
{
   // calculate date difference in days.
   // if days are the same, return zero.
   // if left later than right, return positive.
   // if left earlier than right, return negative.
   int days_diff = 0;
   Pdate lowerbound((std::min)(lhs, rhs));
   Pdate upperbound((std::max)(lhs, rhs));

   while(lowerbound < upperbound)
   {
      days_diff++;
      lowerbound++;
   }

   // negative amounts are possible
   if (lhs < rhs)
      days_diff *= -1;

   return days_diff;
}

int CalcAge(Pdate lhs)
{
   // 12/11/1980 to 12/11/1980: 0
   // 12/11/1980 to 13/11/1980: 0
   // 12/11/1980 to 11/11/1981: 0
   // 12/11/1980 to 12/11/1981: 1
   // 12/11/1980 to 13/11/1981: 1

   int c = 0;
   Pdate today = Pdate(Date(), global_mode); 
   auto a = std::min<Pdate>(lhs, today);
   auto b = std::max<Pdate>(lhs, today);
   
   while(a < b)
   {
      a.mod(1, in_years);
      if(a <= b)
         c++;
   }

   return c;
}

int CalcAge(Pdate lhs, Pdate rhs)
{
   // 12/11/1980 to 12/11/1980: 0
   // 12/11/1980 to 13/11/1980: 0
   // 12/11/1980 to 11/11/1981: 0
   // 12/11/1980 to 12/11/1981: 1
   // 12/11/1980 to 13/11/1981: 1

   int c = 0;
   auto a = std::min<Pdate>(lhs, rhs);
   auto b = std::max<Pdate>(lhs, rhs);
   
   while(a < b)
   {
      a.mod(1, in_years);
      if(a <= b)
         c++;
   }

   return c;
}

utilstr GetTimeSince(Pdate now_date, Pdate then_date)
{
   Pdate g_date = (std::max)(now_date, then_date);
   Pdate l_date = (std::min)(now_date, then_date);

   int y, m, d;
   y = m = d = 0;

   Pdate t_date;

   t_date = l_date;
   while(t_date <= g_date)
   {
      t_date.year++;
      if (t_date <= g_date)
      {
         y++;
         l_date = t_date;
      }
   }

   t_date = l_date;
   while(t_date <= g_date)
   {
      t_date.mod(+1, in_months);
      if (t_date <= g_date)
      {
         m++;
         l_date = t_date;
      }
   }

   t_date = l_date;
   while(t_date <= g_date)
   {
      t_date.mod(+1, in_days);
      if (t_date <= g_date)
      {
         d++;
         l_date = t_date;
      }
   }

   utilstr answer;

   if(y)
      answer += IntToString(y) + " year" + utilstr(y != 1 ? "s" : "");

   if(m)
   {
      if(y)
         answer += ", ";
      answer += IntToString(m) + " month" + utilstr(m != 1 ? "s" : "");
   }

   if(d) // && !(y && m))
   {
      if(y || m)
         answer += ", ";
      answer += IntToString(d) + " day" + utilstr(d != 1 ? "s" : "");
   }

   return answer;
}

// jhaley 20100928: This is done on almost every Prometheus report, so I made a function for it.
// The first day of the month prior to the given date is returned in pair::first. The last day of the same month is
// returned in pair::second.
std::pair<Pdate, Pdate> GetPriorMonthDateRange(Pdate for_date)
{
  Pdate begin_date = for_date;
  begin_date.day   = 1;           // first day of this month
  Pdate end_date = begin_date;    // first day of this month
  begin_date.mod(-1, in_months);  // first day of last month
  end_date.mod(-1, in_days);      // last day of last month

  return std::pair<Pdate, Pdate>(begin_date, end_date);
}

// returns a friendly string representing the difference between two dates 
bool FriendlyDiffDate(utilcstr anchor_datestring, utilcstr relative_datestring, 
                      utilstr &result, utilcstr relative_suffix, 
                      bool force_relative_syntax)
{
   bool to_return = false;
   utilstr class_name = "util";

   result = "";
   Pdate today(Date(), global_mode);
   Pdate anchor_date(anchor_datestring, global_mode);
   bool from_today = (today == anchor_date && !force_relative_syntax);
   bool have_sfx = relative_suffix != "";
   Pdate relative_date(relative_datestring, global_mode);

   if(anchor_date.IsValidDate() && relative_date.IsValidDate())
   {
      // hey, see, i was feeling polite ... this could be move to a slightly prettier 
      // version of that function that -assumes- we're talking about 'today' but what if
      // we were talking about some random date? then we'd want "before" and "after" 
      // rather than "ago" and "hence", and "today", "tomorrow", and "yesterday" would be
      // far messier. ("that day", "the day before", "the day after"?)
      // i don't want to build right now.
      if(anchor_date == relative_date)
      {
         if(from_today)
            result = "today";
         else
         {
            result = "the same day";
            if(have_sfx)
               result += " as " + relative_suffix;
         }
      }
      else if(anchor_date == relative_date + 1)
      {
         if(from_today)
            result = "yesterday";
         else
         {
            result = "the day before";
            if(have_sfx)
               result += " " + relative_suffix;
         }
      }
      else if(anchor_date == relative_date - 1)
      {
         if(from_today)
            result = "tomorrow";
         else
         {
            result = "the day after";
            if(have_sfx)
               result += " " + relative_suffix;
         }
      }
      else if(anchor_date > relative_date)
      {
         // previously these were all written like:
         //result = GetTimeSince(anchor_date, relative_date) + (from_today ? string(" ago") : (" prior" + (have_sfx ? " to " + relative_suffix : string(""))));
         // which worked fine ... most of the time ... and occasionally produced Access Violations for no good reason.  So we write it in expanded form:
         result = GetTimeSince(anchor_date, relative_date);
         if(from_today)
            result += " ago";
         else
         {
            result += " prior";
            if (have_sfx)
               result += " to " + relative_suffix;
         }
      }
      else if(anchor_date < relative_date)
         result = GetTimeSince(anchor_date, relative_date) + (from_today ? utilstr(" from now") : (" since" + (have_sfx ? " " + relative_suffix : utilstr(""))));

      to_return = true;
   }

   return to_return;
}

//TIME----------------------------------------------------------------------

Ptime::Ptime()
{
   hour = 0;
   minute = 0;
   second = 0;
   millisecond = 0;
}

Ptime::Ptime(utilcstr incoming, int mode)
{
   InTime (incoming, mode);
}

void Ptime::InTime(utilcstr incoming, int mode)
{
   int  h  = 0;
   int  m  = 0;
   int  s  = 0;
   int  ms = 0;
   utilstr time = incoming;

   // jhaley 20120914: check for milliseconds
   if(!(mode & withoutmillis))
   {
      size_t dotpos = time.find_last_of(millichar);

      if(dotpos != utilstr::npos)
      {
         ms = StringToInt(time.substr(dotpos+1));
         time = time.substr(0, dotpos);
      }
   }

   //take three chunks out
   septhree (time, timechar, h, m, s);

   //determine military time internally
   // guess whether they mean ampm or not by looking for a p, P, a, or A ...
   if(time.find_first_of("pPaA") != utilstr::npos)
      mode = ampm;

   if (!(mode & military))
   {
      h = h % 12;
      if (time.find_first_of("pP") != utilstr::npos)
         h += 12;
   }

   //store locally
   hour = h;
   minute = m;
   second = s;
   millisecond = ms; // jhaley
}

void Ptime::InTime(const Ptime & incoming)
{
   hour        = incoming.hour;
   minute      = incoming.minute;
   second      = incoming.second;
   millisecond = incoming.millisecond;
}

bool Ptime::IsValidTime () const
{
   if (hour        >  23 || hour        < 0) return false;
   if (minute      >  59 || minute      < 0) return false;
   if (second      >  59 || second      < 0) return false;
   if (millisecond > 999 || millisecond < 0) return false;
   return true;
}

utilstr Ptime::OutTime (int mode) const
{
   utilstr answer = "";
   
   if(mode & military)
   {
      if(mode & left_zeros)
         answer = twodigit(hour);
      else
         answer = IntToString(hour);
      
      answer += timechar + twodigit(minute);
      if(!(mode & withoutsec))
      {
         answer += timechar + twodigit(second);
         if(!(mode & withoutmillis)) // jhaley 20120914
            answer += millichar + threedigit(millisecond);
      }
   }
   else
   {
      int h = hour;
      bool pm = false;
      if(h > 11)
      {
         pm = true;
         h -= 12;
      }
      if(!h) 
         h = 12;
      if(mode & left_zeros)
         answer = twodigit(h);
      else
         answer = IntToString(h);
      answer += timechar + twodigit(minute);
      if(!(mode & withoutsec))
      {
         answer += timechar + twodigit(second);
         if (!(mode & withoutmillis)) // jhaley 20120914
            answer += timechar + threedigit(millisecond);
      }
      answer += (pm) ? " pm" : " am";
   }
   return answer;
}

bool Ptime::operator < (const Ptime & rhs) const
{
   if(hour < rhs.hour) return true;
   if(hour > rhs.hour) return false;
   
   //same hour
   if(minute < rhs.minute) return true;
   if(minute > rhs.minute) return false;
   
   //same minute
   if(second < rhs.second) return true;
   if(second > rhs.second) return false;
   
   //same second
   if(millisecond < rhs.millisecond) return true;
   
   //doesn't matter anymore
   return false;
}

bool Ptime::operator <= (const Ptime & rhs) const
{
   if(hour < rhs.hour) return true;
   if(hour > rhs.hour) return false;
   
   //same hour
   if(minute < rhs.minute) return true;
   if(minute > rhs.minute) return false;
   
   //same minute
   if(second < rhs.second) return true;
   if(second > rhs.second) return false;
   
   //same second
   if(millisecond <= rhs.millisecond) return true;
   
   return false;
}

bool Ptime::operator > (const Ptime & rhs) const
{
   if(hour > rhs.hour) return true;
   if(hour < rhs.hour) return false;
   
   //same hour
   if(minute > rhs.minute) return true;
   if(minute < rhs.minute) return false;
   
   //same minute
   if(second > rhs.second) return true;
   if(second < rhs.second) return false;
   
   //same second
   if(millisecond > rhs.millisecond) return true;
   
   //doesn't matter anymore
   return false;
}

bool Ptime::operator >= (const Ptime & rhs) const
{
   if(hour > rhs.hour) return true;
   if(hour < rhs.hour) return false;

   //same hour
   if(minute > rhs.minute) return true;
   if(minute < rhs.minute) return false;

   //same minute
   if(second > rhs.second) return true;
   if(second < rhs.second) return false;

   //same second
   if(millisecond >= rhs.millisecond) return true;

   //doesn't matter anymore
   return false;
}

bool Ptime::operator != (const Ptime & rhs) const
{
   return (hour != rhs.hour || minute != rhs.minute || 
           second != rhs.second || millisecond != rhs.millisecond);
}

bool Ptime::operator == (const Ptime & rhs) const
{
   return (hour == rhs.hour && minute == rhs.minute &&
           second == rhs.second && millisecond == rhs.millisecond);
}

Ptime Ptime::operator ++ (int)
{
   this->mod(1, in_minutes);
   return (*this);
}

Ptime Ptime::operator -- (int)
{
   this->mod(-1, in_minutes);
   return (*this);
}

int Ptime::mod(int p_distance, int p_metric)
{
   int rolled = 0;
   int ms_distance = p_distance;

   switch(p_metric)
   {
   case in_msec:
      break;
   case in_hours:
      ms_distance *= 60;
      // fall through
   case in_minutes:
      ms_distance *= 60;
      // fall through
   case in_seconds:
      ms_distance *= 1000;
      break;
   default:
      break; // ???
   }

   int delta = 1, hourcap = 0, minutecap = 0, secondcap = 0, mscap = 0;

   if(ms_distance < 0)
   {
      delta     = -1;
      hourcap   = 23;
      minutecap = secondcap = 59;
      mscap     = 999;
   }
   
   while(ms_distance)
   {
      millisecond += delta;
      if(millisecond < 0 || millisecond > 999)
      {
         second += delta;
         if(second < 0 || second > 59)
         {
            minute += delta;
            if(minute < 0 || minute > 59)
            {
               hour += delta;
               if(hour < 0 || hour > 23)
               {
                  rolled += delta;
                  hour = hourcap;
               }
               minute = minutecap;
            }
            second = secondcap;
         }
         millisecond = mscap;
      }
      ms_distance -= delta;
   }
   
   return rolled;
}

int Ptime::mod (const Ptime & p_distance)
{
   int rolled = 0;
   rolled += this->mod(p_distance.hour,        in_hours);
   rolled += this->mod(p_distance.minute,      in_minutes);
   rolled += this->mod(p_distance.second,      in_seconds);
   rolled += this->mod(p_distance.millisecond, in_msec);
   return rolled;
}

int operator - (const Ptime & lhs, const Ptime & rhs)
{
   // calculate time difference in seconds.
   // if times are the same, return zero.
   // if left later than right, return positive.
   // if left earlier than right, return negative.
   int mseconds_diff = 0;
   Ptime lowerbound((std::min)(lhs, rhs));
   Ptime upperbound((std::max)(lhs, rhs));

   mseconds_diff += 3600 * 1000 * (upperbound.hour        - lowerbound.hour);
   mseconds_diff +=   60 * 1000 * (upperbound.minute      - lowerbound.minute);
   mseconds_diff +=        1000 * (upperbound.second      - lowerbound.second);
   mseconds_diff +=               (upperbound.millisecond - lowerbound.millisecond);

   // negative amounts are possible
   if (lhs < rhs)
      mseconds_diff *= -1;

   return mseconds_diff; // jhaley: WARNING: *1000 compared to Prometheus!
}

utilstr GetTimeSince(Ptime now_time, Ptime then_time)
{
   Ptime g_time = (std::max)(now_time, then_time);
   Ptime l_time = (std::min)(now_time, then_time);

   int h, m, s, ms;
   h = m = s = ms = 0;

   Ptime t_time;

   t_time = l_time;
   while(t_time <= g_time)
   {
      t_time.hour++;
      if(t_time <= g_time)
      {
         h++;
         l_time = t_time;
      }
   }

   t_time = l_time;
   while(t_time <= g_time)
   {
      t_time.mod(+1, in_minutes);
      if(t_time <= g_time)
      {
         m++;
         l_time = t_time;
      }
   }

   t_time = l_time;
   while(t_time <= g_time)
   {
      t_time.mod(+1, in_seconds);
      if (t_time <= g_time)
      {
         s++;
         l_time = t_time;
      }
   }

   t_time = l_time;
   while(t_time <= g_time)
   {
      t_time.mod(+1, in_msec);
      if (t_time <= g_time)
      {
         ms++;
         l_time = t_time;
      }
   }

   utilstr answer;

   if(h)
      answer += IntToString(h) + " hour" + utilstr(h != 1 ? "s" : "");

   if(m)
   {
      if(h)
         answer += ", ";
      answer += IntToString(m) + " minute" + utilstr(m != 1 ? "s" : "");
   }

   if(s) // && !(h && m))
   {
      if(h || m)
         answer += ", ";
      answer += IntToString(s) + " second" + utilstr(s != 1 ? "s" : "");
   }

   if(ms)
   {
      if(h || m || s)
         answer += ", ";
      answer += IntToString(ms) + " millisecond" + utilstr(ms != 1 ? "s" : "");
   }

   return answer;
}

// STAMP---------------------------------------------------------------------

Pstamp::Pstamp() : Pdate (), Ptime ()
{
}

Pstamp::Pstamp(utilcstr incoming, int mode)
{
   InStamp(incoming, mode);
}

void Pstamp::InStamp(utilcstr incoming, int mode)
{
   unsigned int splitarea;
   utilstr work_str = StripTrailing(StripLeading(incoming, " "), " ");
   utilstr d = "";
   utilstr t = "";

   if(mode & datefirst)
   {
      splitarea = work_str.find_first_of(" ", 0);
      if(splitarea != utilstr::npos)
      {
         d = work_str.substr(0, splitarea);
         t = work_str.substr(splitarea);
         InDate (d, mode);
         InTime (t, mode);
      }
   }
   else
   {
      splitarea = work_str.find_last_of(" ", work_str.length() - 1);
      if(splitarea != utilstr::npos)
      {
         t = work_str.substr(0, splitarea);
         d = work_str.substr(splitarea);
         InDate (d, mode);
         InTime (t, mode);
      }
   }
}

bool Pstamp::IsValidStamp() const
{
   return IsValidDate() && IsValidTime();
}

utilstr Pstamp::OutStamp(int mode) const
{
   utilstr d = OutDate(mode);
   utilstr t = OutTime(mode);
   utilstr answer = "";

   if(mode & datefirst)
      answer = d + utilstr(" ") + t;
   else
      answer = t + utilstr(" ") + d;

   return answer;
}

bool Pstamp::operator < (const Pstamp & rhs) const
{
   if(Pdate::operator < (rhs)) return true;
   if(Pdate::operator > (rhs)) return false;
   
   // same date
   if(Ptime::operator < (rhs)) return true;
   
   // doesn't matter
   return false;
}

bool Pstamp::operator <= (const Pstamp & rhs) const
{
   if(Pdate::operator < (rhs)) return true;
   if(Pdate::operator > (rhs)) return false;

   // same date
   if(Ptime::operator <= (rhs)) return true;

   // doesn't matter
   return false;
}

bool Pstamp::operator > (const Pstamp & rhs) const
{
   if(Pdate::operator > (rhs)) return true;
   if(Pdate::operator < (rhs)) return false;
   
   // same date
   if(Ptime::operator > (rhs)) return true;
   
   // doesn't matter
   return false;
}

bool Pstamp::operator >= (const Pstamp & rhs) const
{
   if(Pdate::operator > (rhs)) return true;
   if(Pdate::operator < (rhs)) return false;
   
   // same date
   if(Ptime::operator >= (rhs)) return true;
   
   // doesn't matter
   return false;
}

bool Pstamp::operator != (const Pstamp & rhs) const
{
   return (Pdate::operator != (rhs) || Ptime::operator != (rhs));
}

bool Pstamp::operator == (const Pstamp & rhs) const
{
   return (Pdate::operator == (rhs) && Ptime::operator == (rhs));
}

int Pstamp::mod(int p_distance, int p_metric)
{
   int rolled_days = 0;

   if(p_metric & (in_msec | in_seconds | in_minutes | in_hours))
      rolled_days = Ptime::mod(p_distance, p_metric);
   else if(p_metric & (in_days | in_weeks | in_months | in_years))
      Pdate::mod(p_distance, p_metric);

   // make up for time doing stuff by pushing that over to the stamp part
   Pdate::mod(rolled_days, in_days);
   return 0;
}

int Pstamp::mod(const Pstamp & p_distance)
{
   // TODO :
   mod (Ptime::mod(Ptime(p_distance)), in_days);
   Pdate::mod(Pdate(p_distance));
   return 0;
}

int Pstamp::mod (const Pdate & p_distance)
{
   // TODO :
   Pdate::mod(p_distance);
   return 0;
}

int Pstamp::mod (const Ptime & p_distance)
{
   // TODO :
   mod (Ptime::mod(p_distance), in_days);
   return 0;
}

int operator - (const Pstamp & lhs, const Pstamp & rhs)
{
   // calculate time difference in seconds.
   // if times are the same, return zero.
   // if left later than right, return positive.
   // if left earlier than right, return negative.

   // NB: must divide out milliseconds or this can only do a difference of 49
   // days at most, without making appeal to 64-bit arithmetic, which is not 
   // enough to be useful at all. -jhaley

   int seconds_diff = 0;
   int ptime_diff = (Ptime(lhs) - Ptime(rhs)) / 1000;
   int pdate_diff = Pdate(lhs) - Pdate(rhs);
   seconds_diff = ptime_diff + (24*60*60 * pdate_diff);

   return seconds_diff;
}

utilstr GetTimeSince(Pstamp now_stamp, Pstamp then_stamp)
{
   Pstamp g_stamp = (std::max)(now_stamp, then_stamp);
   Pstamp l_stamp = (std::min)(now_stamp, then_stamp);

   int y, mo, d, h, mn, s, ms;
   y = mo = d = h = mn = s = ms = 0;

   Pstamp t_stamp;

   t_stamp = l_stamp;
   while(t_stamp <= g_stamp)
   {
      t_stamp.year++;
      if(t_stamp <= g_stamp)
      {
         y++;
         l_stamp = t_stamp;
      }
   }

   t_stamp = l_stamp;
   while(t_stamp <= g_stamp)
   {
      t_stamp.mod(+1, in_months);
      if(t_stamp <= g_stamp)
      {
         mo++;
         l_stamp = t_stamp;
      }
   }

   t_stamp = l_stamp;
   while(t_stamp <= g_stamp)
   {
      t_stamp.mod(+1, in_days);
      if(t_stamp <= g_stamp)
      {
         d++;
         l_stamp = t_stamp;
      }
   }

   t_stamp = l_stamp; // jhaley 20130110: was missing.
   while(t_stamp <= g_stamp)
   {
      t_stamp.mod(+1, in_hours);
      if(t_stamp <= g_stamp)
      {
         h++;
         l_stamp = t_stamp;
      }
   }

   t_stamp = l_stamp;
   while(t_stamp <= g_stamp)
   {
      t_stamp.mod(+1, in_minutes);
      if(t_stamp <= g_stamp)
      {
         mn++;
         l_stamp = t_stamp;
      }
   }

   t_stamp = l_stamp;
   while(t_stamp <= g_stamp)
   {
      t_stamp.mod(+1, in_seconds);
      if(t_stamp <= g_stamp)
      {
         s++;
         l_stamp = t_stamp;
      }
   }

   t_stamp = l_stamp;
   while(t_stamp <= g_stamp)
   {
      t_stamp.mod(+1, in_msec);
      if(t_stamp <= g_stamp)
      {
         ms++;
         l_stamp = t_stamp;
      }
   }

   utilstr answer;

   if(y)
      answer += IntToString(y) + " year" + utilstr(y != 1 ? "s" : "");

   if(mo)
   {
      if(y)
         answer += ", ";
      answer += IntToString(mo) + " month" + utilstr(mo != 1 ? "s" : "");
   }

   if(d) //&& !(y && mo))
   {
      if(y || mo)
         answer += ", ";
      answer += IntToString(d) + " day" + utilstr(d != 1 ? "s" : "");
   }

   if(h)
   {
      if(y || mo || d)
         answer += ", ";
      answer += IntToString(h) + " hour" + utilstr(h != 1 ? "s" : "");
   }

   if(mn)
   {
      if(y || mo || d || h)
         answer += ", ";
      answer += IntToString(mn) + " minute" + utilstr(mn != 1 ? "s" : "");
   }

   if(s) // && !(h && mn))
   {
      if(y || mo || d || h || mn)
         answer += ", ";
      answer += IntToString(s) + " second" + utilstr(s != 1 ? "s" : "");
   }

   if(ms)
   {
      if(y || mo || d || h || mn || s)
         answer += ", ";
      answer += IntToString(ms) + " millisecond" + utilstr(s != 1 ? "s" : "");
   }
   
   return answer;
}

// END DATE TIME TIMESTAMP CLASS DEFINITIONS---------------------------------

// START STRING MANIPULATION FUNCTIONS---------------------------------------

// Like performs a basic regular expression check. A is an expression which
// must be found in B, and match B entirely. If B is longer than what A is
// mean to express, LikeString returns false. The same goes for A. The
// two special characters (matchany, matchone) are #define'd at the top of
// the .h file. They are set to % and _, like interbase. If they were
// changed to * and ?, this function would act like an MS-DOS string matching
// function on the command line.
bool LikeString(utilcstr a, utilcstr b)
{
   bool does_match = true;
   unsigned int i = 0;
   bool all_b_used = false;
   
   while(i < a.length() && i < b.length() && does_match)
   {
      if(a[i] == match_any)
      {
         // last character is a %, don't recurse
         if(i == a.length() - 1)
         {
            i++; // goes to the end of a, gets out of the loop
            all_b_used = true;
         }
         else
         {
            bool matching_sub = false;
            for(unsigned int j = i; j < b.length(); j++)
            {
               utilstr subquery = a.substr(i + 1);
               utilstr subthing = b.substr(j);
               if (LikeString(subquery, subthing))
                  matching_sub = true;
            }
            if (!matching_sub) does_match = false;
            i = a.length();
            all_b_used = true;
         }
      }
      else if(a[i] == match_one)
      {
         i++;
      }
      else if(a[i] == b[i])
      {
         i++;
      }
      else
      {
         does_match = false;
      }
   }
   
   // make sure all of the matched expression is used up in matching
   if(!all_b_used && i != b.length())
      does_match = false;
   
   // make sure all of the expression is used up, except for trailing %
   if(i != a.length() && a.find_first_not_of("%", i) != utilstr::npos)
      does_match = false;

   return does_match;
}

// SentenceString attempts to capitalize strings in a way that makes sense
// in the context of a sentence, that is, any first character following a
// period, exclamation mark, question mark, or double quote, and any number
// of white spaces.
utilstr SentenceString(utilcstr incoming)
{
   utilstr answer = LowercaseString(incoming);
   char prevchar = '.';
   for (unsigned int i = 0; i < answer.length(); i++)
   {
      if((prevchar == '.' || prevchar == '!' || prevchar == '?' || prevchar == '\"') &&
         (answer[i] >= 'a' && answer[i] <= 'z'))
      {
         answer[i] += ('A' - 'a');
      }
      if(answer[i] != ' ' && answer[i] != '\n' && answer[i] != '\t' && answer[i] != '\a' &&
         answer[i] != '\b' && answer[i] != '\f' && answer[i] != '\r' && answer[i] != '\v')
      {
         prevchar = answer[i];
      }
   }

   answer = ReplaceStringWithString(answer, " i ", " I ");
   return answer;
}

utilstr SpecialNameString(utilcstr incoming)
{
   // look for ^O'xxxx, ^Macxxxx, ^Mcxxxx, and ^I., ^Ii., ^Iii., ^Iv., ^V., ^Vi., etc.
   // -->      ^O'Xxxx, ^MacXxxx, ^McXxxx, and ^I., ^II., ^III., ^IV., ^V., ^VI., etc.
   // where ^ is the beginning of a chunk, and . is whitespace.
   // ... or something like that.

   // heh, let's add a first space, so we can look for spaces ... yeah, that sounds good.
   // and then take it off. yup. that'll be fine, right? what about the whole 'single blank
   // space' thingie? that's an ordering question, yup.
   // TODO :
   utilstr answer;
   return answer;
}

// Transformation function that does not transform :-)
utilstr RawString(utilcstr incoming)
{
   return incoming;
}

//PrettyString will uppercase the first letter of every distinct word, as
//defined by a white space, slash, or dash.
utilstr PrettyString(utilcstr incoming)
{
   utilstr answer = LowercaseString(incoming);
   char prevchar = ' ';
   for(unsigned int i = 0; i < answer.length(); i++)
   {
      if((prevchar == ' ' || prevchar == '/' || prevchar == '-' || prevchar == '(' || prevchar == ')' || prevchar == '.') &&
         (answer[i] >= 'a' && answer[i] <= 'z'))
      {
         answer[i] += ('A' - 'a');
      }
      prevchar = answer[i];
   }
   return answer;
}

//Lowercase every character of a string.
utilstr LowercaseString(utilcstr incoming)
{
   utilstr answer = incoming;
   unsigned int len = answer.length();
   for(unsigned int i = 0; i < len; i++)
   {
      if(answer[i] >= 'A' && answer[i] <= 'Z')
         answer[i] -= ('A' - 'a');
   }
   return answer;
}

// "blah" becomes "%b%l%a%h%" ... dropped into a like clause in stupid cases like
// insurance policies where they're really inconsistent about punctuation and we want to
// return too many matches rather than too few....
utilstr PowerSearchString(utilcstr incoming)
{
   utilstr result = "%";
   for(unsigned int i = 0; i < incoming.length(); i++)
   {
      result = result + incoming[i] + "%";
   }
   return result;
}

// "blah" becomes "b% l% a% h%" ... dropped into a like clause to attempt to match
// initials... eg they put in OHCA and we want to return Oklahoma Health Care Authority
// this one matches the first word beginning at the beginning of the string
utilstr InitialsSearchString1(utilcstr incoming)
{
   utilstr result = "";
   for(unsigned int i = 0; i < incoming.length(); i++)
   {
      if(incoming[i] != ' ')
         result = result + incoming[i] + "% ";
      else
         result = result + "%";
   }
   result = result.substr(0, result.length()-1);
   return result;
}

// "blah" becomes "% b% l% a% h%" ... dropped into a like clause to attempt to match
// initials... eg they put in OHCA and we want to return Oklahoma Health Care Authority
// this one matches the first word starting anywhere in the middle of the string
utilstr InitialsSearchString2(utilcstr incoming)
{
   utilstr result = "% ";
   for(unsigned int i = 0; i < incoming.length(); i++)
   {
      if(incoming[i] != ' ')
         result = result + incoming[i] + "% ";
      else
         result = result + "%";
   }
   result = result.substr(0, result.length()-1);
   return result;
}

//Uppercase every character of a string.
utilstr UppercaseString(utilcstr incoming)
{
   utilstr answer = incoming;
   unsigned int len = answer.length();
   for(unsigned int i = 0; i < len; i++)
   {
      if(answer[i] >= 'a' && answer[i] <= 'z')
         answer[i] += ('A' - 'a');
   }
   return answer;
}

//Uppercase the first letter, without touching the rest
utilstr UppercaseFirstChar(utilcstr incoming)
{
   utilstr answer = incoming;
   if(!answer.empty())
   {
      if(answer[0] >= 'a' && answer[0] <= 'z')
         answer[0] += ('A' - 'a');
   }
   return answer;
}

//Lowercase the first letter, without touching the rest
utilstr LowercaseFirstChar(utilcstr incoming)
{
   utilstr answer = incoming;
   if(!answer.empty())
   {
      if(answer[0] >= 'A' && answer[0] <= 'Z')
         answer[0] -= ('A' - 'a');
   }
   return answer;
}

//replace any occurence of a certain character in a string into another character.
utilstr ReplaceCharWithChar(utilcstr incoming, char from, char to)
{
   utilstr answer = "";
   answer = incoming;
   if(from != to)
   {
      unsigned int match = answer.find(from, 0);
      while(match != utilstr::npos)
      {
         answer[match] = to;
         match = answer.find(from, match);
      }
   }
   return answer;
}

//replace any occurence of a string inside a string with another string.
//correctness verified (but not since Paul added recursive_collapsing, really.  *cough*
utilstr ReplaceStringWithString(utilcstr incoming, utilcstr from, utilcstr to, bool recursive_collapsing)
{
   if(from.length() == 0 || from == to) 
      return incoming; // nothing to do
   
   if(from.length() == 1 && to.length() == 1)
      return ReplaceCharWithChar(incoming, from[0], to[0]); // faster

   if(recursive_collapsing && (to.find(from, 0) != utilstr::npos))
      throw ("ReplaceStringWithString detected a request that would result in an infinite loop.  Aborted.");

   utilstr answer = "";
   unsigned int match   = incoming.find(from, 0);
   unsigned int len     = incoming.length();
   unsigned int fromlen = from.length();
   unsigned int lastbeg = 0;
   bool done = false;
   
   while(match != utilstr::npos && !done)
   {
      answer += incoming.substr(lastbeg, match-lastbeg);
      if(recursive_collapsing) // skip over things that would cause multiple matches
      {
         while(incoming.substr(match+1, from.length())==from)
            match += 1;
      }
      answer += to;
      if(match + fromlen == len)
      {
         done = true;
      }
      else
      {
         lastbeg = match + fromlen;
         match = incoming.find(from, lastbeg);
      }
   }
   
   if(!done) 
      answer += incoming.substr(lastbeg);
   
   // next bit's a hack and should probably be a while loop outside the stuff above, but ...
   // ReplaceStringWithString("ABABBA", "BA", "A", true)
   // returns AABA rather than the AAA it should (because we're only looking for matches forward, not where we've been...
   while(recursive_collapsing && answer.find(from) != utilstr::npos)
      answer = ReplaceStringWithString(answer, from, to, recursive_collapsing);

   return answer;
}

//replace any occurrence of any one of a group of characters with a string.
//correctness verified
utilstr ReplaceAnyCharWithString(utilcstr incoming, utilcstr characters, utilcstr to)
{
   utilstr answer;
   unsigned int match   = incoming.find_first_of(characters, 0);
   unsigned int len     = incoming.length();
   unsigned int fromlen = 1;
   unsigned int lastbeg = 0;
   bool done = false;

   while(match != utilstr::npos && !done)
   {
      answer += incoming.substr(lastbeg, match-lastbeg);
      answer += to;
      
      if(match + fromlen == len)
      {
         done = true;
      }
      else
      {
         lastbeg = match + fromlen;
         match = incoming.find_first_of(characters, lastbeg);
      }
   }
   
   if(!done) 
      answer += incoming.substr(lastbeg);

   return answer;
}

// replaces occurrences of whole words with other strings, from a map.  used by FormatAddress()
// I think it should handle recursive cases thrown at it, but I didn't explicitly test that.
utilstr ReplaceWordsWithWords(utilcstr incoming, utilcmapstrs replacement_map)
{
   utilstr result_string;
   utilstr split_chars     = " `~=_+!@#$%^&*()[]\\;',./{}|:\"<>?\n\t"; // - excluded on purpose
   unsigned int word_begin = incoming.find_first_not_of(split_chars, 0);
   unsigned int word_end   = incoming.find_first_of(split_chars, word_begin);
   
   result_string = incoming.substr(0, word_begin); // initial chunk, if string starts with stripped characters
   
   while(word_end != utilstr::npos)
   {
     utilstr word = incoming.substr(word_begin, word_end - word_begin);
     utilmapstrs::const_iterator found_word = replacement_map.find(word);
     
     if(found_word != replacement_map.end())
       result_string += found_word->second; // found a match, replace it
     else
       result_string += word; // no match, keep existing word
     
     // find next word
     word_begin = incoming.find_first_not_of(split_chars, word_end);
     
     if(word_begin == utilstr::npos) // only stripped characters remain
       result_string += incoming.substr(word_end);
     else // at least one more word
       result_string += incoming.substr(word_end, word_begin - word_end); // add stripped characters
     
     word_end = incoming.find_first_of(split_chars, word_begin);
   }
   
   if(word_begin != utilstr::npos) // one last word
   {
     utilstr word = incoming.substr(word_begin);
     utilmapstrs::const_iterator found_word = replacement_map.find(word);
     if (found_word != replacement_map.end())
       result_string += found_word->second; // found a match, replace it
     else
       result_string += word; // no match, keep existing word
   }

   return result_string;
}

//remove any occurrence of any character in a group of characters from a string.
//correctness verified
utilstr StripAll(utilcstr incoming, utilcstr characters)
{
   utilstr answer = "";
   
   if(incoming.length())
   {
      unsigned int match = incoming.find_first_not_of(characters, 0);
      unsigned int until;
      bool done = false;
      
      while(match != utilstr::npos && !done)
      {
         until = incoming.find_first_of (characters, match);
         if(until != utilstr::npos)
         {
            answer += incoming.substr(match, until-match);
            match = incoming.find_first_not_of(characters, until);
         }
         else
         {
            answer += incoming.substr(match);
            done = true;
         }
      }
   }

   return answer;
}

//remove all identical characters from the leading edge of a string if the
//first character matches a character in the supplied set.
//correctness verified
utilstr StripLeading(utilcstr incoming, utilcstr characters)
{
   utilstr answer = "";

   if(incoming.length())
   {
      if(incoming.find_first_of(characters, 0) == 0)
      {
         utilstr leading = incoming.substr(0,1);
         unsigned int match = incoming.find_first_not_of(leading, 0);
         if(match != utilstr::npos)
            answer = incoming.substr(match);
      }
      else 
         answer = incoming;
   }

   return answer;
}

//removes all identical characters from the end of a string if the last character
//matches a character in the supplied set
//correctness verified
utilstr StripTrailing(utilcstr incoming, utilcstr characters)
{
   utilstr answer = "";
   
   if(incoming.length())
   {
      if(incoming.find_last_of(characters, incoming.length() - 1) == incoming.length() - 1)
      {
         utilstr trailing = incoming.substr(incoming.length() - 1,1);
         unsigned int match = incoming.find_last_not_of(trailing, incoming.length() - 1);
         if(match != utilstr::npos)
            answer = incoming.substr(0, match + 1);
      }
      else 
         answer = incoming;
   }

   return answer;
}

// Pad a string with the given character, to the given length.  You can pad on the left, on the
// right, or both (center given text).  If final_size is less than the length of the incoming
// string, the string will be truncated on the left, right, or (not yet implemented) both sides,
// depending on how_to_pad.
utilstr PadString(utilcstr incoming, pad_string_types how_to_pad, char pad_with, int final_size)
{ 
   // Paul's code
   utilstr result = incoming;

   switch(how_to_pad)
   {
   case pad_left:
      {
         int diff = final_size - incoming.size();
         if (diff > 0) // we've got padding to do!
         {
            utilstr pad = "";
            pad.resize(diff, pad_with);
            result = pad + incoming;
         }
         else if (diff < 0) // we've got truncation to do!
         {
            result = incoming.substr(abs(diff));
         }
         break;
      }
   case pad_right:
      {
         int diff = final_size - incoming.size();
         if (diff > 0) // we've got padding to do!
         {
            utilstr pad = "";
            pad.resize(diff, pad_with);
            result = incoming + pad;
         }
         else if (diff < 0) // we've got truncation to do!
         {
            result = incoming.substr(0, final_size);
         }
         break;
      }
   case pad_to_center:
      {
         int diff = final_size - incoming.size();
         if (diff > 0) // we've got padding to do!
         {
            utilstr lpad = "", rpad = "";
            lpad.resize(int(diff / 2.0), pad_with);
            rpad.resize(int(diff / 2.0 + 0.5), pad_with);
            result = lpad + incoming + rpad;
         }
         else if (diff < 0) // we've got truncation to do!
         {
            // truncate evenly from both sides?  I'm feeling lazy...
            /* doing nothing might actually be a desired (at least default) behavior ... I would not expect
            a padding function to truncate if it's already longer than what I specified ... maybe as an
            optional extra parameter though.  but the stuff above truncates left or right based on how_to_pad
            which still seems contrary to the name.  I mean, it's called PadString, not FitString....  :-\  */
         }
         break;
      }
   }

   return result;
}

// jhaley 20110412: Hard to believe this didn't exist already O_o
// Collapses consecutive space characters, and also changes tabs into single spaces. Line breaks are not affected.
utilstr CollapseSpaces(utilcstr input)
{
   utilstr return_str;
   size_t i, len = input.length();
   bool on_space = false;

   for(i = 0; i < len; i++)
   {
      if(input[i] == ' ' || input[i] == '\t')
      {
         if(!on_space) // Not sitting on a sequence of spaces currently?
         {
            return_str += ' ';
            on_space = true;   // We are now.
         }
         // Otherwise this is a consecutive space; do nothing
      }
      else
      {
         return_str += input[i];
         on_space = false;    // Any other character breaks the run of whitespace.
      }
   }

   return return_str;
}

//END STRING MANIPULATION FUNCTIONS-----------------------------------------

//START FORMAT MANIPULATION FUNCTIONS---------------------------------------

//convert 1,2,4 bytes of character data into integer value (binary)
utilstr ConvertBinary(utilcstr incoming)
{
   utilstr answer = "";
   unsigned int len = incoming.length();
   //assuming that an int is 4 bytes wide, we pad the data to the
   //left with char 0 and assume the result is to be an unsigned int
   char bytearray[4];

   if(len == 1)
   {
      bytearray[0] = 0;
      bytearray[1] = 0;
      bytearray[2] = 0;
      bytearray[3] = incoming[0];
   }
   else if(len == 2)
   {
      bytearray[0] = 0;
      bytearray[1] = 0;
      bytearray[2] = incoming[0];
      bytearray[3] = incoming[1];
   }
   else if(len == 4)
   {
      bytearray[0] = incoming[0];
      bytearray[1] = incoming[1];
      bytearray[2] = incoming[2];
      bytearray[3] = incoming[3];
   }

   answer = IntToString(*(unsigned int *)(bytearray));
   return answer;
}

//convert an AM/PM time to 24 hour time. assumes "h:mm:ss a" input. might not
//be right for every occasion. use with moderation. blank on error.
utilstr ConvertTime(utilcstr incoming)
{
   utilstr answer = "";
   //from ' 9:05:00 A? ' to '21:05:00'
   bool pm = (incoming.find_first_of("pP") != utilstr::npos);
   unsigned int first_num;
   unsigned int first_col;
   unsigned int second_col;
   unsigned int after_sec;
   utilstr stringhours   = "";
   utilstr stringminutes = "";
   utilstr stringseconds = "";

   first_num = incoming.find_first_of("0123456789");
   if(first_num < incoming.length() - 1 && first_num != utilstr::npos)
   {
      first_col = incoming.find_first_of(":", first_num + 1);
      if(first_col < incoming.length() - 1 && first_col != utilstr::npos)
      {
         second_col = incoming.find_first_of(":", first_col + 1);
         if(second_col < incoming.length() - 1 && second_col != utilstr::npos)
         {
            stringhours   = IntToString(StringToInt(incoming.substr(first_num, first_col - first_num)));
            stringminutes = IntToString(StringToInt(incoming.substr(first_col + 1, second_col - first_col - 1)));
            after_sec     = incoming.find_first_not_of("0123456789", second_col + 1);
            
            if (after_sec == utilstr::npos)
               stringseconds = incoming.substr(second_col + 1);
            else
               stringseconds = incoming.substr(second_col + 1, after_sec - second_col - 1);
            
            if(pm) 
               stringhours = IntToString(12 + (StringToInt(stringhours) % 12));
            else  
               stringhours = IntToString(StringToInt(stringhours) % 12);
            
            if(stringhours.length() < 2) 
               stringhours = utilstr("0") + stringhours;
            
            if(stringminutes.length() < 2) 
               stringminutes = utilstr("0") + stringminutes;
            
            if(stringseconds.length() < 2) 
               stringseconds = utilstr("0") + stringseconds;
            
            answer = stringhours + ":" + stringminutes + ":" + stringseconds;
         }
      }
   }

   return answer;
}

// convert a 2-digit year to a 4-digit, with the following format change:
// mm/dd/yy -> yyyy-mm-dd (paradox to interbase conversion)
// not usually what you want. assumes too much. takes a two-digit year spec
// below which years are in the 2000 range, otherwise converted to 1900.
// blank on error.
// three digit year becomes 2036 as a joke (And indicator)
utilstr ConvertDate(utilcstr incoming, int y2kcutoff)
{
   utilstr answer = "";
   //from ' 12/23/97 ' to '1997-12-23'
   unsigned int first_num;
   unsigned int first_slash;
   unsigned int second_slash;
   unsigned int after_sec;
   utilstr stringmonth = "";
   utilstr stringday   = "";
   utilstr stringyear  = "";

   first_num = incoming.find_first_of("0123456789");
   if(first_num < incoming.length() - 1 && first_num != utilstr::npos)
   {
      first_slash = incoming.find_first_of("/-.", first_num + 1);
      if(first_slash < incoming.length() - 1 && first_slash != utilstr::npos)
      {
         second_slash = incoming.find_first_of("/-.", first_slash + 1);
         if(second_slash < incoming.length() - 1 && second_slash != utilstr::npos)
         {
            stringmonth = IntToString(StringToInt(incoming.substr(first_num, first_slash - first_num)));
            stringday   = IntToString(StringToInt(incoming.substr(first_slash + 1, second_slash - first_slash - 1)));
            after_sec   = incoming.find_first_not_of("0123456789", second_slash + 1);
            
            if(after_sec == utilstr::npos)
               stringyear = incoming.substr(second_slash + 1);
            else
               stringyear = incoming.substr(second_slash + 1, after_sec - second_slash - 1);

            stringyear = IntToString(StringToInt(stringyear));
            
            if(stringyear.length() < 2) 
               stringyear = utilstr("0") + stringyear;
            
            if(stringmonth.length() < 2) 
               stringmonth = utilstr("0") + stringmonth;
            
            if(stringday.length() < 2) 
               stringday = utilstr("0") + stringday;
            
            if(stringyear.length() == 3) 
               stringyear = "2036";
            
            if(stringyear.length() < 3)
            {
               if(StringToInt(stringyear) < y2kcutoff) 
                  stringyear = utilstr("20") + stringyear;
               else
                  stringyear = utilstr("19") + stringyear;
            }
            
            answer = stringyear + "-" + stringmonth + "-" + stringday;
         }
      }
   }
   return answer;
}

//converts paradox timestamp to interbase timestamp. involves a time and date
//conversion using other functions, but also a reversal -
// h:mm:ss ?? m/dd/yy -> yyyy-mm-dd hh:mm:ss
//good format for ordering.
//blank on error.
utilstr ConvertStamp(utilcstr incoming, int y2kcutoff)
{
   //from ' 11:25:41 A? 2/18/1999 ' to '1999-02-18 11:25:41'
   utilstr answer = "";
   utilstr newtime = "";
   utilstr newdate = "";
   unsigned int middlespace;

   utilstr work_str = StripTrailing(incoming, " ");
   middlespace = work_str.find_last_of(" ", work_str.length() - 1);
   
   if(middlespace != utilstr::npos)
   {
      newtime = ConvertTime (work_str.substr(middlespace));
      newdate = ConvertDate (work_str.substr(0, middlespace), y2kcutoff);
      answer = newdate + utilstr(" ") + newtime;
   }

   return answer;
}

//converts a text field of F or N to 0, T or Y to 1 for use in an integer
//field. blank on error.
utilstr ConvertBool(utilcstr incoming)
{
   utilstr answer = "";
   utilstr work_str = UppercaseString(incoming);
   
   if(work_str == "F" || work_str == "N" || work_str == "0")
      answer = "0";
   else if (work_str == "T" || work_str == "Y" || work_str == "1" || work_str == "M")
      answer = "1";
   
   return answer;
}

//END FORMAT MANIPULATION FUNCTIONS------------------------------------------

//START INTEGER AND FLOAT TO STRING TRANSLATIONS AND MANIPULATIONS-----------

//returns true if a string contains only digits 0-9 and/or a - sign.
bool IsInt(utilcstr incoming)
{
   // symbols come before digits, always
   if(incoming.find_first_not_of(ints) != utilstr::npos)
      return false;
   
   unsigned int last_symb = incoming.find_last_of ("+-");
   unsigned int first_dig = incoming.find_first_not_of ("-+");
   
   if(first_dig != utilstr::npos)
   {
      if (last_symb != utilstr::npos)
         if (last_symb > first_dig)
            return false;
   }
   else if(last_symb != utilstr::npos)
      return false;

   // allow blank, but don't allow symbols without digits!
   // if digits, and if symbols, then only allow symbols -before- digits
   return true;
}

//returns true if a string contains only digits 0-9, a decimal point, and/or
//a hyphen.
bool IsFloat(utilcstr incoming)
{
   // symbols come before digits, always
   if(incoming.find_first_not_of(floats) != utilstr::npos)
      return false;
   
   unsigned int last_symb = incoming.find_last_of ("+-");
   unsigned int first_dig = incoming.find_first_of (uints);
   unsigned int last_dig  = incoming.find_last_not_of (uints);
   unsigned int decimal_p = incoming.find ('.');
   
   if(first_dig != utilstr::npos)
   {
      if(last_symb != utilstr::npos)
      {
         if(last_symb > first_dig)
            return false;
      }
      if(decimal_p != utilstr::npos)
      {
         if(decimal_p < first_dig || decimal_p > last_dig)
            return false;
      }
   }
   else if (last_symb != utilstr::npos)
      return false;
   
   // allow blank, but don't allow symbols without digits!
   // if digits, and if symbols, then only allow symbols -before- digits
   return true;
}

//convert float to string using C libraries.
utilstr FloatToString(float incoming, int right_num)
{
   utilstr answer = "";
   char floatnumber[30];
   sprintf(floatnumber, utilstr("%1." + IntToString(right_num) + "f").c_str(), incoming);
   answer = floatnumber;
   return answer;
}

//convert float to string using C libraries. -- shows "up to" right_num decimals rather than "exactly" right_num decimals
utilstr FloatToStringApprox(float incoming, int right_num)
{
   utilstr answer = "";
   char floatnumber[30];
   sprintf(floatnumber, utilstr("%1." + IntToString(right_num) + "g").c_str(), incoming);
   answer = floatnumber;
   return answer;
}

//convert string to float using C libraries.
float StringToFloat(utilcstr incoming)
{
   float answer;
   answer = static_cast<float>(atof(incoming.c_str()));
   return answer;
}

//convert int to string using C libraries.
utilstr IntToString(int incoming)
{
   utilstr answer = "";
   char floatnumber[30];
   sprintf(floatnumber, "%1i", incoming);
   answer = floatnumber;
   return answer;
}

//convert int to string using C libraries.
utilstr UIntToString(unsigned int incoming)
{
   utilstr answer = "";
   char floatnumber[30];
   sprintf(floatnumber, "%1u", incoming);
   answer = floatnumber;
   return answer;
}

//convert string to int using C libraries.
int StringToInt(utilcstr incoming)
{
   int answer;
   answer = atoi(incoming.c_str());
   return answer;
}

utilstr IntToHexString(int incoming, int padto, bool prepend0x)
{
   utilstr answer = "";
   char chars[33];
   
   sprintf(chars, "%1X", incoming);
   answer = chars;
   
   if((size_t)padto > answer.length())
      answer = PadString(answer, pad_left, '0', padto);
   
   if(prepend0x)
      answer = "0x" + answer;
   
   return answer;
}

int HexStringToInt(utilcstr incoming)
{
   utilstr temp = incoming;
   if(temp.substr(0, 2) == "0x" || temp.substr(0, 2) == "0X") // jhaley: allow 0X too
      temp = temp.substr(2);

   char *endpt;
   long n = strtoul(temp.c_str(), &endpt, 16);
   
   if(*endpt == 0)
      return n;
   else
      return 0; // on error return 0?  same behavior as atoi / StringToInt()
}

// jhaley 20110816: Input validation for hexadecimal integers (unsigned only)
bool IsHexInt(utilcstr incoming, int maxbits)
{
   size_t maxlen = maxbits / 4; // Ex: 32-bit integer == 8 characters (4 bits per hex char)

   // symbols come before digits, always
   if(incoming.find_first_not_of(hexints) != utilstr::npos)
      return false;

   // If has an "X" or "x" character:
   // * First char must be '0'
   // * Second char must be X or x
   // * No other char must be an X or x
   if(incoming.find_first_of("Xx") != utilstr::npos)
   {
      if(incoming.length() > maxlen + 2)
         return false; // Too long

      if(incoming.length() < 3)
         return false; // Not long enough to be an 0x-style hex number

      if(incoming[0] != '0' || (incoming[1] != 'X' && incoming[1] != 'x'))
         return false; // No leading zero, or x in the wrong position

      if(incoming.substr(2).find_first_of("Xx") != utilstr::npos)
         return false; // Has more than one x
   }
   else if(incoming.length() > maxlen)
      return false; // Too long

   // allow blank, but don't allow symbols without digits!
   // if digits, and if symbols, then only allow symbols -before- digits
   return true;
}


utilstr IntToRGBString(int incoming)
{
   utilstr answer = "";
   char floatnumber[30];
   sprintf(floatnumber, "%1X", incoming);
   answer = floatnumber;
   answer = PadString(answer, pad_left, '0', 6);  // pad with zeros to 6 characters; remember, still in
   answer = FlipRGB(answer);                      // reverse RGB so pad on left
   return answer;
}

utilstr FlipRGB(utilcstr incoming)
{
   utilstr answer = "";
   if(incoming.length() == 6)
      answer = incoming.substr(4,2) + incoming.substr(2,2) + incoming.substr(0,2);
   return answer;
}

int RGBStringToInt(utilcstr incoming)
{
   int to_return = 0;
   utilstr incoming_str = incoming;
   incoming_str = FlipRGB(incoming_str);
   sscanf(incoming_str.c_str(), "%X", &to_return); // name does not imply this behavior??  maybe bool treat_as_color could be an optional extra param?
   return to_return;
}

enum states_checking_float 
{
   state_float_none, 
   state_float_dollar, 
   state_float_sign, 
   state_float_significant, 
   state_float_dot, 
   state_float_decimal
};

bool IsAmount(utilcstr incoming, int left_num, int right_num)
{
   states_checking_float curr_state = state_float_none;
   int  count_significant = 0;
   int  count_decimals    = 0;

   for(unsigned int i = 0; i < incoming.length(); i++)
   {
      switch(incoming[i])
      {
      case '$':
         if(curr_state == state_float_none)
            curr_state = state_float_dollar;
         else
            return false; // "$ encountered while not in state_float_none";
         break;
      case ' ':
         if (curr_state != state_float_none && curr_state != state_float_dollar && curr_state != state_float_sign)
            return false; // "Space encountered while not in state_float_none, state_float_dollar, or state_float_sign";
         break;
      case '+': 
      case '-':
         if(curr_state == state_float_none || curr_state == state_float_dollar)
            curr_state = state_float_sign;
         else
            return false; // "+ or - enountered while not in state_float_none or state_float_dollar";
         break;
      case '0': 
      case '1': 
      case '2': 
      case '3': 
      case '4': 
      case '5': 
      case '6': 
      case '7': 
      case '8': 
      case '9':
         if(curr_state == state_float_sign || curr_state == state_float_significant || curr_state == state_float_dollar || curr_state == state_float_none)
         {
            curr_state = state_float_significant;
            count_significant++;
         }
         else if(curr_state == state_float_dot || curr_state == state_float_decimal)
         {
            curr_state = state_float_decimal;
            count_decimals++;
         }
         else
            return false; // "Digit enountered while not expecting a digit";
         break;
      case ',':
         if(curr_state != state_float_significant)
            return false; // "Comma enountered while not in state_float_significant";
         break;
      case '.':
         if(curr_state == state_float_significant)
            curr_state = state_float_dot;
         else
            return false; // "Dot enountered while not in state_float_significant";
         break;
      default:
         return false; // "Unknown character '" + string(1, incoming[i]) + "' encountered";
      }
   }

   if(curr_state != state_float_none && curr_state != state_float_significant && curr_state != state_float_decimal)
      return false; // "Did not end in either state_float_significant or state_float_decimal"

   // it seems that interbase doesn't care if you have lots of stuff -after-
   // a decimal place... but too much before drives it batty!

   if(count_significant > left_num)
      return false; // "Too many significant digits found"

   return true;
}

// END INTEGER AND FLOAT TO STRING TRANSLATIONS AND MANIPULATIONS-------------

int BoolToInt(bool incoming)
{
   return (incoming == true) ? 1 : 0;
}

//---------------------------------------------------------------------------
utilstr IntToBoolString (int incoming, bool_types how)
{
   utilstr answer;

   if(how == yes_no)
   {
      switch(incoming)
      {
      case 0:  answer = "No";  break;
      case 1:  answer = "Yes"; break;
      default: answer = "";
      }
   }
   else if(how == true_false)
   {
      switch(incoming)
      {
      case 0:  answer = "False"; break;
      case 1:  answer = "True";  break;
      default: answer = "";
      }
   }
   else if(how == zero_one)
   {
      switch(incoming)
      {
      case 0:  answer = "0"; break;
      case 1:  answer = "1"; break;
      default: answer = "";
      }
   }
   else
      answer = "undefined type";

   return answer;
}
//---------------------------------------------------------------------------
utilstr BoolToBoolString (bool incoming, bool_types how)
{
   return IntToBoolString(BoolToInt(incoming), how);
}

//---------------------------------------------------------------------------
//ASHLEY'S CODE - ASHLEY'S CODE - ASHLEY'S CODE - ASHLEY'S CODE - ASHLEY'S CODE
//---------------------------------------------------------------------------

utilstr AddOptions(int searchOption, utilcstr field)
{
   // add % chars to do a like on a field for a select
   // 0 = contains     '%field%'
   // 1 = ends with    '%field'
   // 2 = exact match  'field'
   // 3 = starts with  'field%'

   utilstr returnString = "";
   if(field != "")
   {
      utilstr work_str = UppercaseString(field);
      returnString = ReplaceStringWithString(work_str, "'", "''");
      switch (searchOption)
      {
      case 0: returnString = "'%" + returnString + "%'";  break;
      case 1: returnString = "'%" + returnString + "'";  break;
      case 2: returnString = "'" + returnString + "'";   break;
      case 3: returnString = "'" + returnString + "%'";  break;
      }
   }
   else
      returnString = "''";

   return returnString;
}

//PrettySSN will insert dashes into a string expected to be a social security
//number, if such a string happens to be 9 characters long.
utilstr PrettySSN(utilcstr incoming)
{
   utilstr returnString;
   utilstr first;
   utilstr second;
   utilstr third;

   if(incoming.length() == 9)
   {
      first = incoming.substr(0, 3);
      second = incoming.substr(3, 2);
      third = incoming.substr(5, 4);
      returnString = first + "-" + second + "-" + third;
      return returnString;
   }
   else
      return incoming;
}

utilstr PrettyDate(utilcstr incoming)
{
   if(!incoming.empty() && Pdate(incoming, global_mode).IsValidDate())
      return Pdate(incoming, global_mode).OutDate(global_mode | left_zeros);
   else
      return incoming;
}

utilstr TwoDecimals(utilcstr incoming)
{
   utilstr returnString = "0.00";
   utilstr incomingStripped = StripLeading(incoming, "0");

   if(IsInt(incoming))
   {
      if(incomingStripped == "")
         returnString = "0.00";
      else if(incomingStripped.length() == 1)
         returnString = "0.0" + incomingStripped;
      else if(incomingStripped.length() == 2)
         returnString = "0." + incomingStripped;
      else
      {
         returnString = incomingStripped.substr(0, incomingStripped.length() - 2) + "." +
            incomingStripped.substr(incomingStripped.length() - 2, 2);
      }
   }
   return returnString;
}

utilstr ConvertStupidNumber(utilcstr incoming)
{
   utilstr returnString = "";
   char last_char = incoming.substr(incoming.length()-1, 1)[0];
   bool is_negative = false;
   int number = 0;

   if((last_char < 'S') && (last_char >= 'A'))
      number = ((last_char - 'A') % 9) + 1;

   if(last_char > 'I')
      is_negative = true;
   if(last_char == '{')
      is_negative = false;

   returnString = incoming.substr(0, incoming.length()-1) + IntToString(number);
   returnString = TwoDecimals(returnString);

   if(is_negative)
   {
      if (returnString != "0.00")
         returnString = "-" + returnString;
   }

   return returnString;
}

// takes a phone number from the database which should have only digits
// if the phone number has 10 digits, return the string (999) 999-9999
// else if the phone number hs 7 digits, return the string 999-9999
// else return the string that was passed
// we can assume that the number entered in the database only contains digits
//
// jhaley 20110715: Shared logic between phone number formatting routines moved here.
//
static utilstr FormatPhoneNumberSub(utilcstr incoming, utilcstr dash, utilcstr space)
{
   utilstr return_string = incoming;

   if(incoming.length() == 11)
   {
      if(incoming[0] == '1')
      {
         return_string = incoming.substr(0, 1) + dash + incoming.substr(1, 3) + dash +
            incoming.substr(4, 3) + dash + incoming.substr(7, 4);
      }
   }
   else if(incoming.length() == 10)
   {
      return_string = "(" + incoming.substr(0, 3) + ")" + space + incoming.substr(3, 3) + dash +
         incoming.substr(6, 4);
   }
   else if(incoming.length() == 7)
   {
      return_string = incoming.substr(0, 3) + dash + incoming.substr(3, 4);
   }
   return return_string;
}

// Normal phone number formatting
utilstr FormatPhoneNumber(utilcstr incoming)
{
   return FormatPhoneNumberSub(incoming, "-", " ");
}

// jhaley 20110715: Format a phone number in a manner that won't break across lines in reports. Necessitated by the
// addition of the word "Diabetes" to appointment reminder letters, which pushed the phone number across two lines and
// looked positively nasty.
utilstr FormatPhoneNumberNoBreak(utilcstr incoming)
{
   // Use en-dash (0x96) and non-breaking space (0xA0), which are in the Windows/ANSI character set (0xA0 is also UTF)
   return FormatPhoneNumberSub(incoming, "\x96", "\xA0");
}

// takes an address and tarts it up a bit
utilstr FormatAddress (utilcstr incoming)
{
   InitUspsAbbrMap(); // does so only if not already done
   utilstr return_string = PrettyString(incoming);
   utilstr chars_to_strip = "`~=_+!@$%^&*()[]\\;',.{}|:\"<>?"; // excluding - and #, which we're keeping

   return_string = StripAll(return_string, chars_to_strip);
   return_string = ReplaceWordsWithWords(return_string, usps_abbr_map);

   // a couple of extras that won't get picked up as they're not single whole words:
   return_string = " " + return_string + " ";
   return_string = ReplaceStringWithString(return_string, " P O "," PO ");
   return_string = ReplaceStringWithString(return_string, " N E "," NE ");
   return_string = ReplaceStringWithString(return_string, " N W "," NW ");
   return_string = ReplaceStringWithString(return_string, " S E "," SE ");
   return_string = ReplaceStringWithString(return_string, " S W "," SW ");
   return_string = StripTrailing(StripLeading(return_string, " "), " ");

   return return_string;
}

// Mary P. (Williams) Hickens-Smith = MPWHS
utilstr AcronymString(utilcstr incoming)
{
   utilstr stupid, acceptable_chars, simpler, answer;
   stupid = incoming;
   acceptable_chars = utilstr(alphas) + utilstr(upper_alphas);
   
   // if it's an acceptable character, we use it; else we use space
   for(size_t i = 0; i < stupid.length(); i++)
      simpler += acceptable_chars.find(stupid.substr(i,1)) != utilstr::npos ? stupid.substr(i,1) : utilstr(" "); 
   
   simpler = ReplaceStringWithString(simpler, "  ", " ", true);
   
   // modified from SplitStringToVec
   utilstr separator = " ";
   int lastmatch = 0 - separator.size();
   unsigned int mainmatch = simpler.find(separator, 0);
   utilstr chunk;
   
   while(mainmatch != utilstr::npos)
   {
      chunk = simpler.substr(lastmatch + separator.size(), mainmatch - (lastmatch + separator.size() - 1) - 1);
      answer += chunk.substr(0, 1);
      lastmatch = mainmatch;
      mainmatch = simpler.find(separator, lastmatch + separator.size());
   }

   chunk = simpler.substr(lastmatch + separator.size());
   answer += chunk.substr(0, (std::min)((unsigned int)1, chunk.length()));
   
   return UppercaseString(answer);
}

utilstr PrettySql(utilcstr incoming)
{
   utilstr working_sql = incoming;
   working_sql = ReplaceStringWithString(working_sql, "select", "\nselect");
   working_sql = ReplaceStringWithString(working_sql, "from", "\nfrom");
   working_sql = ReplaceStringWithString(working_sql, "inner join", "\ninner join");
   working_sql = ReplaceStringWithString(working_sql, "left join", "\nleft join");
   working_sql = ReplaceStringWithString(working_sql, "right join", "\nright join");
   working_sql = ReplaceStringWithString(working_sql, "working_sql, left outer join", "\nleft outer join");
   working_sql = ReplaceStringWithString(working_sql, "right outer join", "\nright outer join");
   working_sql = ReplaceStringWithString(working_sql, "natural join", "\nnatural join");
   working_sql = ReplaceStringWithString(working_sql, "full outer join", "\nfull outer join");
   working_sql = ReplaceStringWithString(working_sql, "cross join", "\ncross join");
   working_sql = ReplaceStringWithString(working_sql, "where", "\nwhere");
   working_sql = ReplaceStringWithString(working_sql, "group by", "\ngroup by");
   working_sql = ReplaceStringWithString(working_sql, "order by", "\norder by");
   return ReplaceStringWithString(StripTrailing(StripLeading(working_sql, "\n"), "\n"), "\n\n", "\n", true);
}

// uppercases the string and escapes apostrophes
utilstr SafeSQLString(utilcstr incoming, bool preserve_fieldname_case)
{
   utilstr return_string = incoming;
   if(!preserve_fieldname_case)
      return_string = UppercaseString(return_string);
   return_string = ReplaceStringWithString(return_string, "'", "''");
   return_string = StripLeading(return_string, " ");
   return_string = StripTrailing(return_string, " ");
   return return_string;
}

utilstr FormatZipCode(utilcstr incoming)
{
   utilstr return_string = incoming;

   if(return_string.length() == 9)
      return_string = return_string.substr(0, 5) + "-" + return_string.substr(5, 4);
   
   return return_string;
}

utilstr StripAllExcept(utilcstr incoming, utilcstr to_keep)
{
   utilstr return_string = incoming;
   
   if(incoming != "")
   {
      utilstr all_chars = 
         "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
         "`~1234567890-=_+!@#$%^&*()[]\\;',./{}|:\"<>?\n\t ";
      utilstr to_strip = StripAll(all_chars, to_keep);
      return_string    = StripAll(incoming, to_strip);
   }

   return return_string;
}

//---------------------------------------------------------------------------
utilstr RightNow(bool compact)
{
   utilstr answer;
   utilstr rawdate;
   Pstamp rightnow;

   rawdate = Now(); 
   rightnow.InStamp(rawdate, datefirst | mdy | yyyy | military);
   
   if(!compact) // the format interbase/firebird uses
      answer = rightnow.OutStamp(datefirst | mdy | yyyy | military);
   else // fully qualified date/time stamp that sorts properly (used by, e.g., IHS/NDW's HL7 file naming convention)
      answer = StripAllExcept(rightnow.OutStamp(datefirst | ymd | yyyy | left_zeros | military), uints);

   return answer;
}

utilstr CurrentDate()
{
   return Pdate(Date(), global_mode).OutDate(mdy | yyyy);
}

utilstr CurrentTime()
{
   return Ptime(Time(), global_mode).OutTime(military | withsec);
}

Pdate CurrentPdate()
{
   return Pdate(Date(), global_mode);
}

Ptime CurrentPtime()
{
   return Ptime(Time(), global_mode);
}

Pstamp CurrentPstamp()
{
   Pstamp to_return;
   to_return.InDate(CurrentPdate());
   to_return.InTime(CurrentPtime());
   return to_return;
}

//--------------------------------------------------------------------------
//ASHLEY'S CODE - ASHLEY'S CODE - ASHLEY'S CODE - ASHLEY'S CODE - ASHLEY'S CODE
//--------------------------------------------------------------------------

int RoundNearest(double D)
{
   int answer = static_cast<int>(D);
   if(fabs(D - (double)answer) >= 0.50)
   {
      if(D < 0.0)
         answer--;
      else
         answer++;
   }
   return answer;
}
//--------------------------------------------------------------------------
void SplitStringToVec(utilcstr incoming, utilcstr separator, utilvecstr &vec)
{
   int lastmatch = 0 - separator.size();
   unsigned int mainmatch = incoming.find(separator, 0);

   while(mainmatch != utilstr::npos)
   {
      vec.push_back(incoming.substr(lastmatch + separator.size(), mainmatch - (lastmatch + separator.size() - 1) - 1));

      lastmatch = mainmatch;
      mainmatch = incoming.find(separator, lastmatch + separator.size());
   }
   
   vec.push_back(incoming.substr(lastmatch + separator.size()));
}
//--------------------------------------------------------------------------
void SplitStringToSet(utilcstr incoming, utilcstr separator, utilsetstr &incoming_set)
{
   int lastmatch = 0 - separator.size();
   unsigned int mainmatch = incoming.find(separator, 0);
   while (mainmatch != utilstr::npos)
   {
      incoming_set.insert(incoming.substr(lastmatch + separator.size(), mainmatch - (lastmatch + separator.size() - 1) - 1));

      lastmatch = mainmatch;
      mainmatch = incoming.find(separator, lastmatch + separator.size());
   }
   incoming_set.insert(incoming.substr(lastmatch + separator.size()));
}
//--------------------------------------------------------------------------
utilstr VecToDelimString(utilcvecstr p_vec, utilcstr left, utilcstr right, utilcstr between)
{
   utilstr return_string;
   
   for(utilvecstr::const_iterator itr = p_vec.begin(); itr != p_vec.end(); itr++)
      return_string += (left + *itr + right + between);
   
   return_string = return_string.substr(0, return_string.length() - between.length());
   return return_string;
}
//--------------------------------------------------------------------------
utilstr VecToCommaString(utilcvecstr p_vec)
{
   return VecToDelimString(p_vec, "", "", ", ");
}
//--------------------------------------------------------------------------
bool VecMapColumnToVec(utilcvecmap p_vecmap, utilvecstr &p_vec, utilcstr field, bool include_blank)
{
   bool result = true;

   // loop through vecmap
   for(utilvecmap::const_iterator vec_itr = p_vecmap.begin(); vec_itr != p_vecmap.end(); vec_itr++)
   {
      // find the right column, if it exists
      utilmapstrs::const_iterator map_itr = vec_itr->find(field);
      
      if(map_itr != vec_itr->end())
      {
         // add it to the column values vector
         if (map_itr->second != "" || include_blank)
            p_vec.push_back(map_itr->second);
      }
      else
         // column missing (on at least one row)
         result = false;
   }

   return result;
}
//--------------------------------------------------------------------------
/* 
   DONE: This function should be renamed, as its name currently implies that it does the same thing the 
   other *ToDelimString functions do, but for a vecmap, which it does not.  What it actually does is 
   -extract- a column from a vecmap, and give it to you as a delim string.  Fix should be easy, as this
   function is only used one place in the code, and it's commented out.  Just feeling lazy right now.  
  
   The great thing about having a compiler that doesn't suck is that making changes that require a full 
   rebuild are far less onerous.  Go go gadget TwineCompile.  
*/
utilstr VecMapColumnToDelimString(utilcvecmap p_vecmap, utilcstr field, utilcstr left, utilcstr right, utilcstr between)
{
   // build a vector that contains all the values for the requested column
   utilvecstr column_to_delim;

   // TODO: make the bool on the end here a parameter passed to VecMapColumnToDelimString
   // previous default was true, but VecMapColumnToDelimString isn't used anywhere, so it doesn't matter
   if(VecMapColumnToVec(p_vecmap, column_to_delim, field, false))
   {
      // now we can pass our existing function the vector of values   
      return VecToDelimString(column_to_delim, left, right, between);
   }
   else // oops?
      return "";
}
//--------------------------------------------------------------------------
utilstr VecMapColumnToCommaString(utilcvecmap p_vecmap, utilcstr field)
{
   return VecMapColumnToDelimString(p_vecmap, field, "", "", ", ");
}
//--------------------------------------------------------------------------
utilstr SetToDelimString(utilcsetstr p_set, utilcstr left, utilcstr right, utilcstr between)
{
   utilstr return_string;
   for(utilsetstr::const_iterator itr = p_set.begin(); itr != p_set.end(); itr++)
      return_string += (left + *itr + right + between);
   return_string = return_string.substr(0, return_string.length() - between.length());
   return return_string;
}
//--------------------------------------------------------------------------
utilstr SetToCommaString(utilcsetstr p_set)
{
   return SetToDelimString(p_set, "", "", ", ");
}
//--------------------------------------------------------------------------
utilstr ListToDelimString(utilcliststr p_list, utilcstr left, utilcstr right, utilcstr between)
{
   utilstr return_string;
   for(utilliststr::const_iterator itr = p_list.begin(); itr != p_list.end(); itr++)
      return_string += (left + *itr + right + between);
   return_string = return_string.substr(0, return_string.length() - between.length());
   return return_string;
}
//--------------------------------------------------------------------------
utilstr ListToCommaString(utilcliststr p_list)
{
   return ListToDelimString(p_list, "", "", ", ");
}
//--------------------------------------------------------------------------
utilstr MapToDelimString(utilcmapstrs p_map, utilcstr left_outer, utilcstr right_outer, utilcstr between_pair,
  utilcstr left_key, utilcstr right_key, utilcstr left_value, utilcstr right_value, utilcstr between_key_value)
{
   utilstr return_string;

   for(utilmapstrs::const_iterator itr = p_map.begin(); itr != p_map.end(); itr++)
      return_string += (left_outer + left_key + itr->first + right_key + between_key_value + left_value + itr->second + right_value + right_outer + between_pair);

   return_string = return_string.substr(0, return_string.length() - between_pair.length());
   return return_string;
}
//--------------------------------------------------------------------------
utilstr MapToCommaString(utilcmapstrs p_map)
{
   return MapToDelimString(p_map, "", "", ", ", "", "", "", "", "=");
}
//--------------------------------------------------------------------------
void SKVToPreparedInsert(utilcmapmap skv_map, utillistvec &output_data)
{
   utilvecstr one_row;
   one_row.resize(3);
   for(utilmapmap::const_iterator i_skv = skv_map.begin(); i_skv != skv_map.end(); i_skv++)
   {
      one_row[0] = i_skv->first;
      for(utilmapstrs::const_iterator i_row = i_skv->second.begin(); i_row != i_skv->second.end(); i_row++)
      {
         one_row[1] = i_row->first;
         one_row[2] = i_row->second;
         output_data.push_back(one_row);
      }
   }
}
//---------------------------------------------------------------------------
bool IsInvalidDate(utilcstr date_entered)
{
   return ((!Pdate(date_entered, mdy | yy).IsValidDate()) && (date_entered != ""));
}
//---------------------------------------------------------------------------
utilstr KeywordParseString(utilcstr incoming, utilcstr fieldname)
{
   utilstr outgoing;
   utilstr current_substring;
   keyword_statement current_statement;
   bool is_first = true;
   unsigned int string_index1 = 0;
   int string_index2 = 0;
   int length;

   while(string_index1 != utilstr::npos)
   {
      if(incoming.length() == 0)
         return utilstr("");
      else
      {
         string_index2 = incoming.find(utilstr(" "), string_index1+1);
         length = string_index2 - string_index1;
         current_substring = incoming.substr(string_index1, length);

         if(current_substring[0] == ' ')
         {
            if(current_substring[1] == '!')
               current_statement.not_words.push_back(current_substring.substr(2, (current_substring.length() - 1)));
            else
               current_statement.and_words.push_back(current_substring.substr(1, (current_substring.length() - 1)));
         }
         else
         {
            if(current_substring[0] == '!')
               current_statement.not_words.push_back(current_substring.substr(1, (current_substring.length() - 1)));
            else
               current_statement.and_words.push_back(current_substring);
         }
      }
      string_index1 = string_index2;
   }

   for(unsigned int i = 0; i < current_statement.not_words.size(); i++)
   {
      if(is_first)
      {
         outgoing += utilstr(" " + fieldname + " not containing '" + current_statement.not_words[i] + "' ");
         is_first = false;
      }
      else
         outgoing += utilstr(" and " + fieldname + " not containing '" + current_statement.not_words[i] + "' ");
   }
   
   for(unsigned int i = 0; i < current_statement.and_words.size(); i++)
   {
      if(is_first)
      {
         outgoing += utilstr(" " + fieldname + " containing '" + current_statement.and_words[i] + "' ");
         is_first = false;
      }
      else
         outgoing += utilstr(" and " + fieldname + " containing '" + current_statement.and_words[i] + "' ");
   }

   return outgoing;
}
//---------------------------------------------------------------------------
bool IsBlankOrZero(utilcstr id_field_value)
{
   return (id_field_value == "" || id_field_value == "0" || !IsInt(id_field_value) || StringToInt(id_field_value) == 0);
}
//---------------------------------------------------------------------------
// NOTE:  despite the name, these can be used for any delimited text files
//---------------------------------------------------------------------------
// Converts a raw line from a CSV file into a vector of string (field) values, based on quote and delim
// characters specified and whether or not you want it to watch for double (quote) characters w/i quoted fields.
// Input: raw_line, quote_char, delim_char, watch_doubles;  Output: vecCSV
// Lifted from the File Upload utility and revised to have clearer variable names & comments, handle the
// double thing as a bool rather than a separate character that must be null or match quote_char, etc.
bool DelimStringToVec(utilcstr raw_line, char quote_char, char delim_char, utilvecstr &vecCSV, bool watch_doubles)
{
   // vecCSV.clear(); // should be doing this on your side, not in function, yeah?

   // parse raw_line into a vector of strings
   bool quoted = false;           // are we within a quoted field?  Behave appropriately.
   unsigned int char_number = 0;  // current position in raw_line during processing
   char this_char;                // current character during processing
   char next_char;                // next character during processing (checking for doubles)
   utilstr string_value = "";     // the value of the current field once we've figured it out

   while(char_number < raw_line.length())
   {
      this_char = raw_line[char_number];
      // get the next character too, in case of doubling
      if(char_number < raw_line.length() - 1)
         next_char = raw_line[char_number + 1];
      else
         next_char = 0;

      // determine appropriate action
      if(quoted)
      {
         if(watch_doubles && this_char == quote_char && next_char == quote_char)
         {
            string_value += quote_char;
            char_number += 2;
         }
         else if(this_char == quote_char)
         {
            if(next_char == delim_char || next_char == 0) // end of field quote == quoted field termination
               quoted = false;
            else
            {
               // middle of field quote == just another character (and improperly formatted CSV, but we'll be 
               // nice, since Excel and some other third-party utilities are in importing these)
               string_value = string_value + this_char; 
            }
            char_number ++;
         }
         else
         {
            string_value += this_char;
            char_number++;
         }
      }
      else
      {
         if(this_char == quote_char)
         {
            if(string_value == "") // beginning of field quote == quoted field entry
               quoted = true;
            else
               string_value = string_value + this_char;
            char_number ++;
         }
         else if(this_char == delim_char)
         {
            vecCSV.push_back(string_value);
            string_value = "";
            char_number++;
         }
         else
         {
            string_value += this_char;
            char_number++;
         }
      }
   } // end while

   vecCSV.push_back(string_value);
   return true;
}
//---------------------------------------------------------------------------
// wrapper for DelimStringToVec
bool CSVtoVecMap(utilcstr filename, char quote_char, char delim_char, utilvecmap &vecmap, utilstr &err_message, bool watch_doubles)
{
   utilvecstr   fields;
   utilvecstr   datas;
   utilmapstrs  field_data_map;
   utilstr      raw_line;
   bool         all_is_well = true;
   std::fstream the_file;
   unsigned int row = 0;

   err_message = "";

   if(!FileExists(filename.c_str())) // FIXME
   {
      err_message = "Input file does not exist";
      all_is_well = false;
   }
   if(all_is_well)
   {
      try
      { 
         the_file.open(filename.c_str(), std::ios::in); 
      }
      catch(...) 
      { 
         all_is_well = false; 
      }

      if(!the_file.is_open() || !all_is_well)
      {
         err_message = "Could not open file";
         all_is_well = false;
      }
      
      try
      { 
         std::getline(the_file, raw_line, '\n'); 
         row++; 
      }
      catch(...) 
      { 
         err_message = "Error reading from input file"; 
         all_is_well = false; 
      }
      
      if(raw_line.length() && raw_line[raw_line.length() - 1] == '\0')
         raw_line = raw_line.substr(0, raw_line.length() - 1);
      
      // first line should contain header row with field names
      if(all_is_well &&
         DelimStringToVec(raw_line, quote_char, delim_char, fields, watch_doubles))
      {
         field_data_map.clear();

         while(!the_file.eof() && all_is_well)
         {
            raw_line = "";
            try
            { 
               getline(the_file, raw_line, '\n'); 
               row++; 
            }
            catch(...)
            { 
               err_message = "Error reading from input file"; 
               all_is_well = false; 
            }
            
            // jhaley 20121128: bug: if length == 0, this would access the string at 4294967295
            if(raw_line.length() && raw_line[raw_line.length() - 1] == '\0')
               raw_line = raw_line.substr(0, raw_line.length() - 1);
            
            datas.clear();
            if(all_is_well && raw_line.length() > 0 && 
               DelimStringToVec(raw_line, quote_char, delim_char, datas, watch_doubles))
            {
               if(datas.size() >= fields.size())
               {
                  for (unsigned int i=0; i<fields.size(); i++)
                     field_data_map[fields[i]] = datas[i];
                  vecmap.push_back(field_data_map);
               }
               else
               {
                  err_message = "Insufficient columns on row " + IntToString(row) + ":\n\n" + raw_line;
                  all_is_well = false;
               }
            }
         }
      }
   }

   the_file.close();
   return all_is_well;
}
//---------------------------------------------------------------------------
// Philip named this "in" because he's a dumbass
// Paul renamed it "is_in" because ... he's a smartass?
// Paul also changed this to const string & to make it faster
// Philip changed it back when he discovered that va_start is not compatible with reference parameters
// (it tries to take the address of the parameter to determine stuff about the parameter array, and ... this throws it off somehow.)
bool is_in(const utilstr *val, ...) 
{                             
   bool answer = false;
   va_list argument_list;
   va_start(argument_list, val);
   char * next_val = va_arg(argument_list, char *);

   while(next_val)
   {
      if(*val == next_val)
      {
         answer = true;
         break;
      }
      next_val = va_arg(argument_list, char *);
   }

   va_end (argument_list);
   return answer;
}
//---------------------------------------------------------------------------
// Split a string similar to A=B into A and B
bool SplitKeyValue(utilcstr incoming, utilstr &key, utilstr &value)
{
   if(incoming.find_first_of("=") != utilstr::npos)
   {
      key = incoming.substr(0, incoming.find_first_of("="));
      value = StripFirst(incoming.substr(incoming.find_first_of("=")));
   }
   else
      return false;

   return true;
}
//---------------------------------------------------------------------------
// Strip off the first character of a string and return a new copy
utilstr StripFirst(utilcstr incoming)
{
   if(incoming.length() > 1)
      return incoming.substr(1, incoming.length() - 1);

   return "";
}
//---------------------------------------------------------------------------
// Strip off the last character of a string and return a new copy
utilstr StripLast(utilcstr incoming)
{
   return incoming.substr(0, incoming.length() - 1);
}
//---------------------------------------------------------------------------
// Returns true if the string is surrounded with [], false otherwise.
bool IsBracketed(utilcstr incoming)
{
   if(incoming.length())
   {
      if(incoming[0] == '[' && incoming[incoming.length() - 1] == ']')
         return true;
   }

   return false;
}
//---------------------------------------------------------------------------
// Compare two strings while converted to uppercase.
bool UppercaseCompare(utilcstr str1, utilcstr str2)
{
   return UppercaseString(str1) == UppercaseString(str2);
}
//---------------------------------------------------------------------------
// Load a text file from disk. Returns null if fails.
char *LoadTextFile(const char *filename)
{
   FILE *f;
   char *buffer;
   size_t size;

   if(!(f = fopen(filename, "rb")))
      return NULL; // can't open file

   fseek(f, 0, SEEK_END);
   size = ftell(f);
   fseek(f, 0, SEEK_SET);

   if(size <= 0)
   {
      fclose(f);
      return NULL; // nothing in the file
   }

   buffer = new char [size + 1]; // size+1 for null termination
   memset(buffer, 0, size + 1);

   if(fread(buffer, 1, size, f) != size)
   {
      delete [] buffer;
      fclose(f);
      return NULL; // can't read the file
   }

   fclose(f);

   return buffer; // success
}
//---------------------------------------------------------------------------
// Load a binary file from disk. Returns null if fails.
unsigned char *LoadBinaryFile(const char *filename, size_t &size)
{
   FILE *f;
   unsigned char *buffer;

   if(!(f = fopen(filename, "rb")))
      return NULL; // can't open file

   fseek(f, 0, SEEK_END);
   size = ftell(f);
   fseek(f, 0, SEEK_SET);

   if(size <= 0)
   {
      fclose(f);
      return NULL; // nothing in the file
   }

   buffer = new unsigned char [size];
   memset(buffer, 0, size);

   if(fread(buffer, 1, size, f) != size)
   {
      delete [] buffer;
      fclose(f);
      return NULL; // can't read the file
   }

   fclose(f);

   return buffer; // success
}
//---------------------------------------------------------------------------
// jhaley 20130611: Allscripts is driving me insane putting spaces in the actual lookup table values.
utilstr StripSurrounding(utilcstr incoming, utilcstr characters)
{
   return StripLeading(StripTrailing(incoming, characters), characters);
}
//---------------------------------------------------------------------------
utilstr GenerateUUID()
{
   std::string res("");

#ifdef WIN32
   UUID uuid;
   memset(&uuid, 0, sizeof(uuid));
   UuidCreate(&uuid);
   RPC_CSTR struuid;
   if((UuidToStringA(&uuid, &struuid) == RPC_S_OK) && struuid)
   {
      res = std::string(reinterpret_cast<const char *>(struuid));
      RpcStringFreeA(&struuid);
   }
#endif
      
   return res;
}
//---------------------------------------------------------------------------
unsigned int SDBMHash(const char *str)
{
   auto ustr = reinterpret_cast<const unsigned char *>(str);
   int c;
   unsigned int h = 0;

   while((c = *ustr++))
      h = c + (h << 6) + (h << 16) - h;

   return h;
}
//---------------------------------------------------------------------------
unsigned int SDBMHash(utilcstr in)
{
   return SDBMHash(in.c_str());
}
//---------------------------------------------------------------------------