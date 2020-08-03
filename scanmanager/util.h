// PAN
#ifndef utilityfunctions
#define utilityfunctions

#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <set>
#include <ctime>

// typedefs

typedef std::string                        utilstr;
typedef utilstr const &                    utilcstr;
typedef std::vector<std::string>           utilvecstr;
typedef utilvecstr const &                 utilcvecstr;
typedef std::set<std::string>              utilsetstr;
typedef utilsetstr const &                 utilcsetstr;
typedef std::list<std::string>             utilliststr;
typedef utilliststr const &                utilcliststr;
typedef std::map<std::string, std::string> utilmapstrs;
typedef utilmapstrs const &                utilcmapstrs;
typedef std::vector<utilmapstrs>           utilvecmap;
typedef utilvecmap const &                 utilcvecmap;
typedef std::map<std::string, utilmapstrs> utilmapmap;
typedef utilmapmap const &                 utilcmapmap;
typedef std::vector<utilvecstr>            utilvecvec;
typedef std::list<utilvecstr>              utillistvec;

// enums

enum bool_types 
{
   true_false, 
   yes_no, 
   zero_one
};

enum pad_string_types 
{
   pad_left, 
   pad_right, 
   pad_to_center
};

// defines

// general purpose bit flag stuff... if using | and &, #define with these
// for consistency and so we can easily spot things in this file that should
// be enums instead!
// <!--  column 25 ...  v  -->
#define  bit_none       0      // no flags set
#define  bit_1          1      // first flag set
#define  bit_2          2      // second flag set
#define  bit_3          4      // third flag set
#define  bit_4          8      // ...
#define  bit_5          16     // ...
#define  bit_6          32     // ...
#define  bit_7          64     // ...
#define  bit_8          128    // a byte's worth of flags!
#define  bit_9          256    // ...
#define  bit_10         512    // ...
#define  bit_11         1024   // ...
#define  bit_12         2048   // ...
#define  bit_13         4096   // ...
#define  bit_14         8192   // ...
#define  bit_15         16384  // ...
#define  bit_16         32768  // an integer's worth of flags!
                               // TODO: go to longint if necessary...

// current - used as a parameter to StripAllExcept, ints mostly
#define ints            "1234567890-+"
#define uints           "0123456789"
#define hexints         "0123456789ABCDEFabcdefXx"
#define floats          "1234567890.+-"
#define ufloats         "1234567890."
#define alphas          "abcdefghijklmnopqrstuvwxyz"
#define alpha_numerics  "abcdefghijklmnopqrstuvwxyz1234567890"
#define upper_alphas         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define upper_alpha_numerics "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"

// current - used by all Pdate/Ptime/Pstamp funcitons as "mode"
// notes   - can generally be cumulated with the pipe symbol
#define unknown         bit_none
#define yy              0
#define yyyy            bit_1
#define mdy             0
#define ymd             bit_2
#define dmy             bit_3
#define ampm            0
#define military        bit_4
#define timefirst       0
#define datefirst       bit_5
#define withsec         0
#define withoutsec      bit_6
#define no_zeros        0
#define left_zeros      bit_7
#define separators      0
#define no_separators   bit_8  // jhaley 20120723 - don't add in datechars
#define withmillis      0
#define withoutmillis   bit_9  // jhaley 20120914 - don't add milliseconds

// current - used by all Pdate/Ptime/Pstamp functions as "metric"
// notes   - cannot be combined. this changes the meaning of "distance"
#define in_undef        bit_none
#define in_seconds      bit_1
#define in_minutes      bit_2
#define in_hours        bit_3
#define in_days         bit_4
#define in_weeks        bit_5
#define in_months       bit_6
#define in_years        bit_7
#define in_msec         bit_8

// current - used by Pdate/Ptime/Pstamp anytime a mode is not
//           specified by the user. reflects generally accepted parameters.
// mostly used by filter/edit_listview for sorting (comes this way from db)
 #define global_mode    yyyy | mdy | military | datefirst

// current - used by Pdate/Ptime/Pstamp as standard date/time separators
#define datechar        '/'
#define datecharalt     '-'
#define timechar        ':'
#define millichar       '.' 
#define y2kcut          1905

// current - used by Like() as meta characters for simple regular expressions.
// notes   - corresponds to interbase/firebird's standard characters.
#define match_any       '%'
#define match_one       '_'

// Date and Time

// Accept dates in one format, output in the other, play...
class Pdate
{
public:
   int year;
   int month;
   int day;
   Pdate();
   Pdate(utilcstr incoming, int mode);
   Pdate(const time_t *time);   // jhaley 20120907
   bool IsValidDate () const;
   void InDate(utilcstr incoming, int mode);
   void InDate(const Pdate & incoming);
   void InDate(const time_t *time); // jhaley 20120907
   utilstr OutDate (int mode) const;
   bool operator <  (const Pdate & rhs) const;
   bool operator <= (const Pdate & rhs) const;
   bool operator >  (const Pdate & rhs) const;
   bool operator >= (const Pdate & rhs) const;
   bool operator != (const Pdate & rhs) const;
   bool operator == (const Pdate & rhs) const;
   Pdate operator ++ (int);
   Pdate operator -- (int);
   int mod(int p_distance, int p_metric);
   int mod(const Pdate & p_distance);
   int DayOfWeek() const;
   Pdate operator += (int rhs);
   Pdate operator -= (int rhs);
   utilstr NameOfDay() const;
   utilstr NameOfMonth() const;
};

// Accept time in one format, output in the other, play...
class Ptime
{
public:
   int hour;
   int minute;
   int second;
   int millisecond;
   Ptime();
   Ptime(utilcstr incoming, int mode);
   bool IsValidTime () const;
   void InTime(utilcstr incoming, int mode);
   void InTime(const Ptime & incoming);
   utilstr OutTime (int mode) const;
   bool operator <  (const Ptime & rhs) const;
   bool operator <= (const Ptime & rhs) const;
   bool operator >  (const Ptime & rhs) const;
   bool operator >= (const Ptime & rhs) const;
   bool operator != (const Ptime & rhs) const;
   bool operator == (const Ptime & rhs) const;
   Ptime operator ++ (int);
   Ptime operator -- (int);
   int mod(int p_distance, int p_metric);
   int mod(const Ptime & p_distance);
};

// A date and time together, to do stuff in formats...
class Pstamp : public Pdate, public Ptime
{
public:
   Pstamp();
   Pstamp(utilcstr incoming, int mode);
   bool IsValidStamp() const;
   void InStamp(utilcstr incoming, int mode);
   utilstr OutStamp (int mode) const;
   bool operator <  (const Pstamp & rhs) const;
   bool operator <= (const Pstamp & rhs) const;
   bool operator >  (const Pstamp & rhs) const;
   bool operator >= (const Pstamp & rhs) const;
   bool operator != (const Pstamp & rhs) const;
   bool operator == (const Pstamp & rhs) const;
   int mod(int p_distance, int p_metric);
   int mod(const Pstamp & p_distance);
   int mod(const Pdate & p_distance);
   int mod(const Ptime & p_distance);
};

// Outside pdate operators
Pdate operator + (const Pdate & lhs, const int & rhs);
Pdate operator - (const Pdate & lhs, const int & rhs);
int   operator - (const Pdate & lhs, const Pdate & rhs);

int     CalcAge(Pdate lhs);
int     CalcAge(Pdate lhs, Pdate rhs);
utilstr GetTimeSince(Pdate now_date, Pdate then_date);
utilstr GetTimeSince(Ptime now_date, Ptime then_date);
utilstr GetTimeSince(Pstamp now_date, Pstamp then_date);
bool    FriendlyDiffDate(utilcstr anchor_datestring, utilcstr relative_datestring, utilstr &result, utilcstr relative_suffix, bool force_relative_syntax);

std::pair<Pdate, Pdate> GetPriorMonthDateRange(Pdate for_date);

// Outside ptime operators
int operator - (const Ptime & lhs, const Ptime & rhs);

// Outside pstamp operators
int operator - (const Pstamp & lhs, const Pstamp & rhs);

// Other date/time/stamp stuff
utilstr RightNow(bool compact = false);
utilstr CurrentDate();
utilstr CurrentTime();
Pdate   CurrentPdate();
Ptime   CurrentPtime();
Pstamp  CurrentPstamp();

// Query
bool    IsInt(utilcstr incoming);
bool    IsFloat(utilcstr incoming);
bool    IsLeapYear(int year);
int     LastDayOf(int month, int year);
bool    LikeString(utilcstr regular_expression_string, utilcstr string_to_search);
bool    IsEnter(char key);
bool    IsAmount(utilcstr incoming, int left_num = 15, int right_num = 2);
bool    IsInvalidDate(utilcstr date_entered);

//strip/pad/replace
utilstr StripTrailing(utilcstr incoming, utilcstr characters);
utilstr StripLeading(utilcstr incoming, utilcstr characters);
utilstr StripSurrounding(utilcstr incoming, utilcstr characters);
utilstr StripAll(utilcstr incoming, utilcstr characters);
utilstr StripAllExcept(utilcstr incoming, utilcstr characters);
utilstr ReplaceCharWithChar(utilcstr incoming, char from, char to);
utilstr ReplaceStringWithString(utilcstr incoming, utilcstr from, utilcstr to, bool recursive_collapsing = false);
utilstr ReplaceAnyCharWithString(utilcstr incoming, utilcstr characters, utilcstr to);
utilstr PadString(utilcstr incoming, pad_string_types how_to_pad, char pad_with, int final_size);
utilstr CollapseSpaces(utilcstr input);
utilstr ReplaceWordsWithWords(utilcstr incoming, utilcmapstrs replacement_map);

int RoundNearest(double D);
void InitUspsAbbrMap();

// Convert
utilstr twodigit(int incoming);
utilstr StringToAmount (utilcstr incoming, int left_num = 15, int right_num = 2);
utilstr AmountToString (utilcstr incoming, int left_num = 15, int right_num = 2);
utilstr PrettyAmountString (utilcstr incoming, bool blank_zero_amounts = true);
utilstr IntToString(int incoming);
utilstr UIntToString(unsigned int incoming);
int     StringToInt(utilcstr incoming);
utilstr IntToHexString(int incoming, int padto=0, bool prepend0x=false);
int     HexStringToInt(utilcstr incoming);
bool    IsHexInt(utilcstr incoming, int maxbits); // jhaley 20110816
utilstr IntToRGBString(int incoming);
int     RGBStringToInt(utilcstr incoming);
utilstr FlipRGB(utilcstr incoming);
utilstr FloatToStringApprox(float incoming, int right_num);
utilstr FloatToString(float incoming, int right_num = 2);
float   StringToFloat(utilcstr incoming);
utilstr RawString (utilcstr incoming);
utilstr SentenceString (utilcstr incoming);
utilstr PrettyString (utilcstr incoming);
utilstr UppercaseFirstChar(utilcstr incoming);
utilstr LowercaseFirstChar(utilcstr incoming);
utilstr PowerSearchString (utilcstr incoming);
utilstr InitialsSearchString1 (utilcstr incoming);
utilstr InitialsSearchString2 (utilcstr incoming);
utilstr LowercaseString (utilcstr incoming);
utilstr UppercaseString(utilcstr incoming);
utilstr ConvertBinary(utilcstr incoming);
utilstr ConvertTime(utilcstr incoming);
utilstr ConvertDate(utilcstr incoming, int y2kcutoff);
utilstr ConvertStamp(utilcstr incoming, int y2kcutoff);
utilstr ConvertBool(utilcstr incoming);
void    septhree(utilcstr incoming, char separator, int &A, int&B, int&C);
utilstr AddOptions(int searchOption, utilcstr field);
utilstr PrettySSN(utilcstr incoming);
utilstr PrettyDate(utilcstr incoming);
utilstr TwoDecimals(utilcstr incoming);
utilstr ConvertStupidNumber(utilcstr incoming);
utilstr FormatPhoneNumber(utilcstr incoming);
utilstr FormatPhoneNumberNoBreak(utilcstr incoming); // jhaley
utilstr FormatZipCode(utilcstr incoming);
utilstr FormatAddress (utilcstr incoming);
utilstr AcronymString(utilcstr incoming);
utilstr PrettySql(utilcstr incoming);
utilstr SafeSQLString(utilcstr incoming, bool preserve_fieldname_case = false);
utilstr IntToBoolString (int incoming, bool_types how = yes_no);
utilstr BoolToBoolString (bool incoming, bool_types how = true_false);
int     BoolToInt(bool incoming);
void    SplitStringToVec(utilcstr incoming, utilcstr separator, utilvecstr &vec);
void    SplitStringToSet(utilcstr incoming, utilcstr separator, utilsetstr &incoming_set);
utilstr VecToDelimString(utilcvecstr p_vec, utilcstr left, utilcstr right, utilcstr between);
utilstr VecToCommaString(utilcvecstr p_vec);
utilstr SetToDelimString(utilcsetstr p_set, utilcstr left, utilcstr right, utilcstr between);
utilstr SetToCommaString(utilcsetstr p_set);
utilstr ListToDelimString(utilcliststr p_list, utilcstr left, utilcstr right, utilcstr between);
utilstr ListToCommaString(utilcliststr p_list);
utilstr MapToDelimString(utilcmapstrs p_map, utilcstr left_outer,utilcstr right_outer, 
                         utilcstr between_pair, utilcstr left_key, utilcstr right_key, utilcstr left_value, 
                         utilcstr right_value, utilcstr between_key_value);
utilstr MapToCommaString(utilcmapstrs p_map);
utilstr VecMapColumnToDelimString(utilcvecmap p_vecmap, utilcstr field, utilcstr left, utilcstr right, utilcstr between);
utilstr VecMapColumnToCommaString(utilcvecmap p_vecmap, utilcstr field);
bool    VecMapColumnToVec(utilcvecmap p_vecmap, utilvecstr &p_vec, utilcstr field, bool include_blank = true);
void    SKVToPreparedInsert(utilcmapmap skv_map, utillistvec &output_data);
utilstr KeywordParseString(utilcstr incoming, utilcstr fieldname);
bool    DelimStringToVec(utilcstr raw_line, char quote_char, char delim_char, utilvecstr &vecCSV, bool watch_doubles);
bool    CSVtoVecMap(utilcstr filename, char quote_char, char delim_char, utilvecmap &vecmap, utilstr &err_message, bool watch_doubles);
bool    IsBlankOrZero(utilcstr id_field_value);

// use with care! takes char* for all other values, use NULL to terminate. example: if (in(some_string, "1", "2", "3", NULL)) {}
bool is_in(const utilstr *val, ...); 

// jhaley 20121129: functions from hl7daemon stringutils, some of which are
// originally from the Prometheus inifile module.

bool UppercaseCompare(utilcstr str1, utilcstr str2);

/** 
 * Split a string similar to A=B into A and B
 * @param[in]  incoming Input string in format A=B
 * @param[out] key Output string which receives the value A
 * @param[out] value Output string which receives the value B
 * @return True if successful, false otherwise.
 */
bool SplitKeyValue(utilcstr incoming, utilstr &key, utilstr &value);

/** 
 * Strip off the first character of a string and return a new copy
 * @param incoming String to format
 * @return Input string minus the first character
 */
utilstr StripFirst(utilcstr incoming);

/** 
 * Strip off the last character of a string and return a new copy
 * @param incoming String to format.
 * @return Input string minus the last character.
 */
utilstr StripLast(utilcstr incoming);

/**
 * Test if the string is surrounded with []
 * @param incoming Input string
 * @return True if input string starts with [ and ends with ], false otherwise.
 */
bool IsBracketed(utilcstr incoming);

/**
 * Load a text file into memory.
 * @param filename Absolute path of file to load.
 * @return If the file could be opened and read successfully, the contents
 *         of the file in full, with an added null terminator. If any error
 *         occurs, NULL will be returned.
 * @warning You must free the results with delete [] when finished with them.
 */
char *LoadTextFile(const char *filename);

/**
 * Load a binary file into memory.
 * @param[in] filename Absolute path of file to load.
 * @param[inout] size Reference to a size_t to receive size of the buffer.
 * @return If the file could be opened and read successfully, the contents
 *         of the file in full. If any error occurs, NULL will be returned
 *         and the contents of size are undefined.
 * @warning You must free the results with delete [] when finished with them.
 */
unsigned char *LoadBinaryFile(const char *filename, size_t &size);

/**
 * Return a UUID string
 * @return UUID in string form
 */
utilstr GenerateUUID();

/**
 * Return the SDBM hash code for a character string
 */
unsigned int SDBMHash(const char *str);

/**
 * Return the SDBM hash code for a string instance
 */
unsigned int SDBMHash(utilcstr in);

template<class T>
T & get_vec_map_column_sum(utilvecmap & data, utilcstr column_name, T & addition_object)
{
   utilstr field_value;
   utilvecmap::iterator i_row;
   utilmapstrs::iterator i_field;
   for(i_row = data.begin(); i_row != data.end(); i_row++)
   {
      i_field = (*i_row).find(column_name);
      if (i_field != (*i_row).end())
      {
         field_value = i_field->second;
         addition_object(field_value);
      }
   }
 
   return addition_object;
}

template <class T>
void VecToSet(const std::vector<T> & input, std::set<T> & output)
{
   output.clear();
   for(std::vector<T>::const_iterator i = input.begin(); i != input.end(); i++)
      output.insert(*i);
};

template <class T>
std::set<T> VecToSet(const std::vector<T> & input)
{
   std::set<T> output;
   output.clear();
   
   for(std::vector<T>::const_iterator i = input.begin(); i != input.end(); i++)
      output.insert(*i);
   
   return output;
};

template <class T>
void SetToVec(const std::set<T> & input, std::vector<T> & output)
{
   output.clear();
   for(std::set<T>::const_iterator i = input.begin(); i != input.end(); i++)
      output.push_back(*i);
};

template <class T>
std::vector<T> SetToVec(const std::set<T> & input)
{
   std::vector<T> output;
   output.clear();
   for(std::set<T>::const_iterator i = input.begin(); i != input.end(); i++)
      output.push_back(*i);
   
   return output;
};

template <class K, class V>
void MergeMap (std::map<K, V> & merge_into, std::map<K, V> & merge_from)
{
   for(std::map<K, V>::iterator i_from = merge_from.begin(); i_from != merge_from.end(); i_from++)
      merge_into[i_from->first] = i_from->second;
}

class numbered_string_generator
{
private:
   unsigned int next_num;
   utilstr base_text;

public:
   numbered_string_generator(utilcstr initial_text, unsigned int initial_num) 
      : next_num(initial_num), base_text(initial_text) 
   {
   }
   
   utilstr operator () ()
   {
      utilstr answer = base_text + IntToString(next_num);
      next_num++;
      return answer;
   }
};

template <class item>
bool dependency_graph(std::map<item, std::set<item>> item_dependencies, 
                      std::vector<std::set<item>> & item_order, bool push_down = false)
{
   // return false if a conflict is found
   // see algorithm notes from the original class, below.
   bool found_this_pass;
   std::set<item> add_this_pass;
   std::map<item, unsigned int>   item_to_band;
   std::map<item, std::set<item>> item_children;
   std::map<item, std::set<item>> original_item_dependencies;
   
   item_order.clear();
   
   if(push_down)
      original_item_dependencies = item_dependencies;

   do
   {
      add_this_pass.clear();
      found_this_pass = false;
      std::map<item, std::set<item> >::iterator i_next = item_dependencies.begin();
      for (map<item, std::set<item> >::iterator i_item = item_dependencies.begin(); i_item != item_dependencies.end(); i_item = i_next)
      {
         i_next = i_item;
         i_next++;

         if(i_item->second.empty())
         {
            add_this_pass.insert(i_item->first);
            item_dependencies.erase(i_item);
         } 
         else
            found_this_pass = true;
      }

      // these dependencies have been fulfilled, remove from waiting list's lists. (confusing to say?)
      for(std::map<item, std::set<item>>::iterator i_item = item_dependencies.begin(); i_item != item_dependencies.end(); i_item++)
      {
         for(std::set<item>::iterator i_added = add_this_pass.begin(); i_added != add_this_pass.end(); i_added++)
            i_item->second.erase(*i_added);
      }

      if(!add_this_pass.empty())
      {
         if(push_down)
         {
            unsigned int band_number = item_order.size();
            for(std::set<item>::iterator i = add_this_pass.begin(); i != add_this_pass.end(); i++)
            {
               item_to_band[*i] = band_number;
               for(std::set<item>::iterator j = original_item_dependencies[*i].begin(); j != original_item_dependencies[*i].end(); j++)
                  item_children[*j].insert(*i);
            }
         }
         item_order.push_back(add_this_pass);
      }
   } 
   while(!add_this_pass.empty());

   // go back and push things to the right if possible
   if(push_down)
   {
      unsigned int max_band = item_order.size() - 1;
      std::set<item>::iterator l;
      for(std::vector<std::set<item>>::reverse_iterator i = item_order.rbegin(); i != item_order.rend(); i++)
      {
         for(std::set<item>::iterator j = i->begin(); j != i->end(); j++)
         {
            unsigned int currently_in = item_to_band[*j];
            unsigned int minimum_child = max_band;
            for(std::set<item>::iterator k = item_children[*j].begin(); k != item_children[*j].end(); k++)
               minimum_child = min(minimum_child, item_to_band[*k]);
            if(minimum_child - 1 > currently_in && minimum_child != max_band)
            {
               unsigned int should_be_in = minimum_child - 1;
               item_to_band[*j] = should_be_in;
            }
         }
      }

      // too many problems with modifying the vector on the fly, so ... 
      // fixing that by just re-inserting crap here. (yes, by that I mean I was an idiot of some sort.)
      for(std::vector<std::set<item>>::iterator i = item_order.begin(); i != item_order.end(); i++)
         i->clear();

      for(std::map<item, unsigned int>::iterator i = item_to_band.begin(); i != item_to_band.end(); i++)
         item_order[i->second].insert(i->first);
   }

   // if we had a pass where nothing was added, but things were waiting, deadlock.
   if(found_this_pass)
      return false;

   return true;
}

struct keyword_statement
{
   utilvecstr and_words;
   utilvecstr not_words;
};

template<class T>
std::set<T> subtract(const std::set<T> & a, const std::set<T> & b)
{
   std::set<T> answer;
   set_difference(a.begin(), a.end(),
                  b.begin(), b.end(),
                  insert_iterator<std::set<T>>(answer, answer.begin()));
   return answer;
}

template<class T>
std::set<T> add(const std::set<T> & a, const std::set<T> & b)
{
   std::set<T> answer;
   set_union(a.begin(), a.end(),
             b.begin(), b.end(),
             insert_iterator<std::set<T>>(answer, answer.begin()));
   return answer;
}

template<class T>
std::set<T> intersect (const std::set<T> & a, const std::set<T> & b)
{
   std::set<T> answer;
   set_intersection(a.begin(), a.end(),
                    b.begin(), b.end(),
                    insert_iterator<std::set<T>>(answer, answer.begin()));
   return answer;
}

template<class K, class V>
V MapValue(std::map<K, V> &M, K &key)
{
   std::map<K, V>::iterator I = M.find(key);
   if(I != M.end()) 
      return I->second;
   else 
      return V();

   // "what in the everloving hell?" I hear you asking.  Well.  The debugger
   // won't let you Watch or Inspect some_map[some_value] with bracket notation,
   // but you CAN do both with a user defined function.  Thus, this.
}
//---------------------------------------------------------------------------

#endif




