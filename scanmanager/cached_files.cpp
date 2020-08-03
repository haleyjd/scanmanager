/*

   Prometheus Stub Launcher

   Cached Files Module

   This is the same as in Prometheus, but is cut down to the parts that are
   relevant to management of local files and the cache directory tree only.

*/

#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>

#ifdef _MSC_VER
#include <direct.h>
#define mkdir _mkdir
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode & S_IFMT) == S_IFDIR))
#endif 
#ifndef S_IRWXU
#define S_IRWXU (S_IREAD | S_IWRITE | S_IEXEC)
#endif
#endif

// jhaley 20120524: Builder 2007 has moved the location and changed the name of mkdir
#if  __BORLANDC__ >= 0x0590
#include <dir.h>
#define mkdir _mkdir
#endif

#include "cached_files.h"

static std::string basePath;

//
// FileCache::SetBasePath
//
// Set the path to use as the base path for other file path operations
//
void FileCache::SetBasePath(const string &path)
{
   basePath = path;
}

//
// FileCache::GetBasePath
//
// Return the base path under which we create the main cache directory
//
string FileCache::GetBasePath(void)
{
   /*
   string ret_path;
   char path_buffer[MAX_PATH+1];
   HRESULT ret_code;

   memset(path_buffer, 0, sizeof(path_buffer));
   ret_code = SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, (LPSTR)path_buffer);
   if(SUCCEEDED(ret_code))
      ret_path = path_buffer;
   else
      ret_path = "."; // default to trying to use the current directory, though it may not work.

   return ret_path;
   */
   return basePath;
}

//
// FileCache::RemoveTrailingSlash
//
// Removes a trailing slash from a path component if it ends in one.
//
string FileCache::RemoveTrailingSlash(const string &source_path)
{
   string ret_str = source_path;

   if(source_path.length() > 0 && source_path.at(source_path.length() - 1) == '\\')
      ret_str = source_path.substr(0, source_path.length() - 1);

   return ret_str;
}

//
// FileCache::RemoveLeadingSlash
//
// Removes a leading slash from a path component, if it begins with one.
//
string FileCache::RemoveLeadingSlash(const string &source_path)
{
   string ret_str = source_path;

   if(source_path.length() > 0 && source_path.at(0) == '\\')
      ret_str = source_path.substr(1);

   return ret_str;
}

//
// FileCache::DirectoryExists
//
// Tests an absolute path for existence as a directory. If test_if_full_perms is true, then the directory's existence
// will be ignored unless the user has full permissions (read/write/execute) to that directory.
//
bool FileCache::DirectoryExists(const string &path, bool test_if_full_perms)
{
   bool dir_exists = false;
   struct stat s;

   string tmp_path = RemoveTrailingSlash(path); // stat doesn't find paths ending with a slash

   if(!stat(tmp_path.c_str(), &s) && S_ISDIR(s.st_mode))
   {
      if(test_if_full_perms)
         dir_exists = ((s.st_mode & S_IRWXU) == S_IRWXU);
      else
         dir_exists = true;
   }

   return dir_exists;
}

//
// FileCache::FileExists
//
// Similar to the above but for a readable disk file rather than a directory. Send in an absolute path.
//
bool FileCache::FileExists(const string &path, const string &file)
{
   bool file_exists = false;
   struct stat s;

   string tmp_path = PathConcatenate(path, file);

   if(!stat(tmp_path.c_str(), &s) && !S_ISDIR(s.st_mode))
      file_exists = true;

   return file_exists;
}

//
// FileCache::FileExistsInCache
//
// Calls the above routine after concatenating the local file cache basepath to the passed-in path; ie., that path
// should be relative to the cache basepath - ex: 'Prometheus\images...', NOT 'C:\Users\FooBar\AppData...'
//
bool FileCache::FileExistsInCache(const string &path, const string &file)
{
   return FileExists(PathConcatenate(GetBasePath(), path), file);
}

//
// FileCache::PathConcatenate
//
// Concatenate two portions of a file path safely with respect to whether either part might end or begin with a slash.
//
string FileCache::PathConcatenate(const string &base, const string &newpart)
{
   string base_trunc    = RemoveTrailingSlash(base);
   string newpart_trunc = RemoveTrailingSlash(RemoveLeadingSlash(newpart));

   return (base_trunc + "\\" + newpart_trunc);
}

//
// FileCache::GetFileSpec
//
// Extract the filename from a path and return the path and filename as a pair.
//
pair<string, string> FileCache::GetFileSpec(const string &full_path)
{
   pair<string, string> parts_to_return;
   size_t last_slash_pos = full_path.find_last_of("\\/"); // find final slash character

   // If found a slash and...            not at beginning, and not at very end then:
   if(last_slash_pos != string::npos && last_slash_pos != 0 && last_slash_pos != full_path.size() - 1)
   {
      parts_to_return.first  = full_path.substr(0, last_slash_pos);  // file path in .first
      parts_to_return.second = full_path.substr(last_slash_pos + 1); // file name in .second
   }
   else
   {
      parts_to_return.first = "";         // no valid path component to return
      parts_to_return.second = full_path; // return full parameter in .second
   }

   return parts_to_return;
}

//
// FileCache::CreateDirectorySingle
//
// Create a directory under the cache base path, non-recursive, provided it doesn't exist already.
//
bool FileCache::CreateDirectorySingle(const string &dir_name)
{
   string concat_path = PathConcatenate(GetBasePath(), dir_name);

   if(DirectoryExists(concat_path, false))
      return true; // already exists

   // Create the directory
   return !mkdir(concat_path.c_str());
}

//
// FileCache::CreateDirectoryRecursive
//
// Creates all components of a file path that might not already exist under the cache base path.
//
bool FileCache::CreateDirectoryRecursive(const string &path)
{
   string trimmed_path = RemoveTrailingSlash(RemoveLeadingSlash(path));
   size_t last_slash   = trimmed_path.find_last_of('\\');

   if(last_slash == string::npos && trimmed_path.length() > 0)
   {
      // Final directory component to create.
      return CreateDirectorySingle(trimmed_path);
   }
   else
   {
      // Recurse to create parent directory first.
      if(!CreateDirectoryRecursive(trimmed_path.substr(0, last_slash)))
         return false;

      // Create this directory.
      return CreateDirectorySingle(trimmed_path);
   }
}

//
// FileCache::DropFromCache
//
// Attempt to delete the local cached copy of a file. Returns true if a file was removed, and false if it didn't exist
// or couldn't be removed for whatever reason.
//
bool FileCache::DropFromCache(const string &filepath, const string &filename)
{
   bool removed_file = false;

   if(FileExistsInCache(filepath, filename))
   {
      string full_path = PathConcatenate(GetBasePath(), PathConcatenate(filepath, filename));
      removed_file = !remove(full_path.c_str());
   }

   return removed_file;
}

//
// FileCache::CopyIntoCache
//
// Copies an external (non-versioned) file into the file cache's base directory.
// "file" should be an absolute path (or at least relative to the working directory); "relativedir" is the destination
// under the file cache base directory where the file should be copied.
//
bool FileCache::CopyIntoCache(const string &file, const string &relativedir)
{
   if(CreateDirectoryRecursive(relativedir))
   {
      pair<string, string> file_parts = GetFileSpec(file);
      string dest_path = PathConcatenate(PathConcatenate(GetBasePath(), relativedir), file_parts.second);

      return (CopyFileA((LPCSTR)file.c_str(), (LPCSTR)dest_path.c_str(), FALSE) != 0);
   }
   else
      return false; // failed to create that path, can't copy the file
}

// EOF


