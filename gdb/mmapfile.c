/* This implememts the mmapfile command for gdb */

#include "defs.h"
#include "infcall.h"
#include "inferior.h"
#include "gdbcore.h"
#include "objfiles.h"
#include "solib.h"
#include "symfile.h"
#include "arch-utils.h"
#include "completer.h"
#include "cli/cli-decode.h"
#include "gdb_assert.h"
#include <fcntl.h>
#include "regcache.h"
#include "regset.h"
#include "sys/mman.h"
#include "filenames.h"
#include <readline/tilde.h>

int count = 1;

int mmap_fileopen (const char *path, const char *string,
                   char **filename_opened);

static void
mmap_command (char *args, int from_tty)
{
        unsigned long int       addr;
        static struct           target_section_table *mmap_data;
        asection                *sec, *vmsec;
        int                     fd, flags;
        char                    *file_pathname, *mmapsec_name, *sec_name;

        off64_t offset  = 0;
        bfd_vma vaddr   = 0;
        size_t size     = 0;
        int i = 0;
        char *arg[4];

        arg[i] = strtok (args," ");

        while (arg[i++] != NULL)
                arg[i] = strtok (NULL, " ");

/* Check if the parameters entered by the user are correct in number and format */

        if (i != 5)
                error (_("MMAP ERROR: Usage: mmapfile <FILENAME> <MAP-ADDRESS> \
<FILE-OFFSET> <LENGTH>"));

/* No point using mmap if gdb is not debugging a core */

        if (core_bfd == NULL)
                error (_("MMAP ERROR: No core file loaded."));

/* Find out if the user entered offset and size in base 16 or base 10 */

        if (arg[2][0] == '0' && arg[2][1] == 'x')
                offset  = strtoll(arg[2], NULL, 16);
        else
                offset  = strtoll(arg[2], NULL, 10);

        if (arg[3][0] == '0' && arg[3][1] == 'x')
                size    = strtoll(arg[3], NULL, 16);
        else
                size    = strtoll(arg[3], NULL, 10);

        addr = parse_and_eval_address (arg[1]);

/* Check if the desired mmap address overlaps with an already existing
   virtual memory address in the core. If so, then display error */

        vmsec   = bfd_get_section_by_name(core_bfd, ".vmdata");

        while (vmsec != NULL)
        {
                if (addr >= vmsec->vma && addr < (vmsec->vma + vmsec->size))

                        error(_("MMAP_ERROR:\nCannot memory map file <%s> at\n\
address 0x%15.lX as it results in an overlap\nuse info target for more info"), arg[0], addr);


                if (strcmp (vmsec->next->name, ".vmdata") == 0)
                        vmsec = vmsec->next;
                else
                        vmsec = NULL;
        }

        if (size == 0)
                error (_("MMAP ERROR: size cannot be 0"));

/* Get file descriptor for the desired file to be mmaped */

        fd = mmap_fileopen (getenv ("PATH"), arg[0],
                                &file_pathname);

        if (fd < 0)
                perror_with_name (arg[0]);

        flags = SEC_ALLOC | SEC_LOAD | SEC_HAS_CONTENTS | SEC_IN_MEMORY;

/* mmap the given file to the desired address/symbol and let vaddr point to it */

        vaddr = (bfd_vma) mmap((void*)addr, size, PROT_READ, MAP_FIXED | MAP_SHARED, fd, offset);

        if ((long) vaddr == -1)
                   error(_("MMAP_ERROR:\nCannot memory map file <%s> at\n\
address 0x%15.lX as it results in an overlap\nuse info target for more info"), arg[0], addr);

/* Get a unique section name and assign it to the new section created in the core */

        mmapsec_name = bfd_get_unique_section_name (core_bfd, "mmap_section", &count);

        sec_name = (char*) malloc (snprintf (NULL, 0, "%s (Mapped to %s)", mmapsec_name, file_pathname));

        sprintf(sec_name, "%s (Mapped to %s)", mmapsec_name, file_pathname);

/* Create a new section in the core_bfd with this unique section name */

        sec = bfd_make_section_anyway_with_flags (core_bfd, sec_name, flags);

/* Fill out the required section contents */

        sec->contents = (unsigned char*) vaddr;
        bfd_set_section_vma (core_bfd, sec, vaddr);
        bfd_set_section_alignment (core_bfd, sec, 0);
        bfd_set_section_size (core_bfd, sec, size);

/* Build a new section table which has contains all the sections present
   in core_bfd and also the newly created mmap section */

        mmap_data = XCNEW (struct target_section_table);

        if (build_section_table (core_bfd,
                                   &mmap_data->sections,
                                   &mmap_data->sections_end))

                    error (_("\"%s\": Can't find sections: %s"),
                              bfd_get_filename (core_bfd), bfd_errmsg (bfd_get_error ()));

        close(fd);
        mmap_core (mmap_data);

        printf_filtered (_("File %s is now mapped\nfrom address %lX to %lX.\n"), file_pathname,
                                                                         addr, addr+size);
}

int mmap_fileopen (const char *path, const char *string,
                   char **filename_opened)
{
  int fd;
  char *filename;
  int alloclen;
  VEC (char_ptr) *dir_vec;
  struct cleanup *back_to;
  int ix;
  char *dir;

  gdb_assert (string != NULL);

  /* A file with an empty name cannot possibly exist.  Report a failure
     without further checking.

     This is an optimization which also defends us against buggy
     implementations of the "stat" function.  For instance, we have
     noticed that a MinGW debugger built on Windows XP 32bits crashes
     when the debugger is started with an empty argument.  */
  if (string[0] == '\0')
    {
      errno = ENOENT;
      return -1;
    }

  if (!path)
    path = ".";

  filename = (char*) alloca (strlen (string) + 1);
          strcpy (filename, string);
          fd = open (filename, O_RDONLY);
          if (fd >= 0)
            goto done;

  /* For dos paths, d:/foo -> /foo, and d:foo -> foo.  */
  if (HAS_DRIVE_SPEC (string))
    string = STRIP_DRIVE_SPEC (string);

  /* /foo => foo, to avoid multiple slashes that Emacs doesn't like.  */
  while (IS_DIR_SEPARATOR(string[0]))
    string++;

  /* ./foo => foo */
  while (string[0] == '.' && IS_DIR_SEPARATOR (string[1]))
    string += 2;

  alloclen = strlen (path) + strlen (string) + 2;
  filename = (char*) alloca (alloclen);
  fd = -1;

  dir_vec = dirnames_to_char_ptr_vec (path);
  back_to = make_cleanup_free_char_ptr_vec (dir_vec);

  for (ix = 0; VEC_iterate (char_ptr, dir_vec, ix, dir); ++ix)
    {
      size_t len = strlen (dir);

      if (strcmp (dir, "$cwd") == 0)
        {
          /* Name is $cwd -- insert current directory name instead.  */
          int newlen;

          /* First, realloc the filename buffer if too short.  */
          len = strlen (current_directory);
          newlen = len + strlen (string) + 2;
          if (newlen > alloclen)
            {
              alloclen = newlen;
              filename = (char*) alloca (alloclen);
            }
          strcpy (filename, current_directory);
        }
      else if (strchr(dir, '~'))
        {
         /* See whether we need to expand the tilde.  */
          int newlen;
          char *tilde_expanded;

          tilde_expanded  = tilde_expand (dir);

          /* First, realloc the filename buffer if too short.  */
          len = strlen (tilde_expanded);
          newlen = len + strlen (string) + 2;
          if (newlen > alloclen)
            {
              alloclen = newlen;
              filename = (char*) alloca (alloclen);
            }
          strcpy (filename, tilde_expanded);
          xfree (tilde_expanded);
        }
      else
        {
          /* Normal file name in path -- just use it.  */
          strcpy (filename, dir);

          /* Don't search $cdir.  It's also a magic path like $cwd, but we
             don't have enough information to expand it.  The user *could*
             have an actual directory named '$cdir' but handling that would
             be confusing, it would mean different things in different
             contexts.  If the user really has '$cdir' one can use './$cdir'.
             We can get $cdir when loading scripts.  When loading source files
             $cdir must have already been expanded to the correct value.  */
          if (strcmp (dir, "$cdir") == 0)
            continue;
        }

      /* Remove trailing slashes.  */
      while (len > 0 && IS_DIR_SEPARATOR (filename[len - 1]))
        filename[--len] = 0;

      strcat (filename + len, SLASH_STRING);
      strcat (filename, string);

          fd = open (filename, O_RDONLY);
          if (fd >= 0)
            break;
    }

  do_cleanups (back_to);

done:
  if (filename_opened)
    {
      /* If a file was opened, canonicalize its filename.  Use  gdb_realpath
         rather than xfullpath to avoid resolving the basename part
         of filenames when the associated file is a symbolic link.  This
         fixes a potential inconsistency between the filenames known to
         GDB and the filenames it prints in the annotations.  */
      if (fd < 0)
        *filename_opened = NULL;
      else if (IS_ABSOLUTE_PATH (filename))
        *filename_opened = gdb_realpath (filename);
      else
        {
          /* Beware the // my son, the Emacs barfs, the botch that catch...  */

          char *f = concat (current_directory,
                            IS_DIR_SEPARATOR (current_directory[strlen (current_directory) - 1])
                            ? "" : SLASH_STRING,
                            filename, (char *)NULL);

          *filename_opened = gdb_realpath (f);
          xfree (f);
        }
    }

  return fd;
}

/* Implement mmap command and add it to the commands list */

extern initialize_file_ftype _initialize_mmap;

void
_initialize_mmap (void)
{
add_com("mmapfile", class_vars, mmap_command, _("\
Add contents of memory mapped files missing from core dump.\n\n\
Usage:\nmmapfile <FILENAME> <MAP-ADDRESS / SYMBOL NAME> <FILE-OFFSET> <LENGTH>\n\
Command is used (mostly while debugging a core) if the core file sections do not contain\n\
memory mapped regions under the vmdata section.\n\
This happens if files are mmap-ed with MMAP_PRIVATE/MAP_SHARED and RO permissions.\n\
Using this command, the user can access data from\n\
the mmap-ed files, which are missing from the core file."));

        add_com_alias ("mmap", "mmapfile", class_files, 1);
}
