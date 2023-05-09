
# Watch Directories File Format

The file format is similar to a `.ini` file. You create a section with
at least a `path=...` parameter to activate a listen on that directory.

# Example

Say you want to copy your `named` zone files that you created under
`/var/lib/bind`. This can be achieved by defining a section as follow:

    [bind]
    path=/var/lib/bind

This tells the `snaprfs` service to copy files that change in `/var/lib/bind`
to any other `snaprfs` currently listening for such changes.

The computers listening for changes would have modes setup to slightly
different values:

    [bind]
    path=/var/lib/bind
    path_mode=receive-only
    delete_mode=apply

In other words, the first computer is viewed as the "primary" and the other
computers as "secondary". At the moment, there is no automatic way to switch
from one type to another. If the "primary" for a folder may auto-switch,
then you probably want to consider using `path_mode=latest` instead.

# Invalid Parameters

If a parameter is invalid (i.e. `delete_mode=unknown`) then the system
generates a recoverable error in your logs and that path is completely
ignored.

It is done that way since this is the safest way to avoid incorrectly
handling your shared files. Without that features, we could end up
overwriting files you wanted to preserve.

# Available Parameters

Each section has a name to distinguish it from other sections. It has no
other purpose. If an error is detected, the section name is shown as a
namespace. For example, if the `path=...` parameter is missing, that
field will be referenced as:

    bind::path

as per the example above.

## Path

The path to the directory to listen for file changes:

    [bind]
    path=/to/a/folder

This parameter is mandatory. If not present or set to just "/", then this
whole section is ignored and an error message is logged.

### Pattern

If you want to only listen for certain files or exclude certain files, you
can use extended GNU glob-like patterns. Just add the pattern at the end of
the path. For example, to copy only the .zone files from our `ipmgr`, you
could use the following path and pattern:

    /var/lib/bind/*.zone

If you want to call all the files but the journal files:

    /var/lib/bind/!(*.jnl)

The supported patterns are:

* Anything (`*`)

  The standard `*` metacharacter can be used to allow for any or no
  character to match your pattern.

* Any One Character (`?`)

  The standard `?` metacharacter can be used to match exactly one character.

* Character Class (`[...]`)

  The standard `[...]` metacharacter class can be used to include matches of
  character ranges (i.e. `[a-z]`) or just a set of characters (`[adkj78]`).
  The class can be negated using the `!` character (`[!a-z]`).

  Note that we check for a corresponding `]` to interpret the `[` character
  as a class introducer

* Extended Patterns (`#(pattern)` where `#` is one of `*?+@!`)

  The GNU fnmatch(3) function supports extended patterns. See that manual
  page for complete details. The `...` is a list of pattern. Patterns
  are separated by the `|` character.

  The `*(...)` accepts zero or more of the patterns.

  The `?(...)` accepts zero or one of the patterns.

  The `+(...)` accepts one or one of the patterns.

  The `@(...)` accepts exactly one of the patterns.

  The `!(...)` accepts input that do not match any of the patterns.

### Limitations

The directory needs to exist at the time the watch is put in place.

It is possible to listen for changes on a specific file. However,
this requires the file to exist and not ever get deleted. This is
why we generally say you should listen to directories.

## Path Mode

`snaprfs` allows for copying files in one direction or both directions.

* `path_mode=send-only` (default)

  In this mode, this computer is the source computer. The other computers
  should use the `receive-only` mode for that same path.

  This is the default mode if the mode is not specified.

* `path_mode=receive-only`

  This computer accepts files from other computers. If the file changes
  locally, nothing happens.

* `path_mode=latest`

  The latest version of a file is shared between computers in your cluster.
  Note that when a file is received, then this has no further effect. Updates
  need to come from local services, not the act of copying the file between
  cluster computers.

## Delete Mode

When a file gets deleting on a computer, we need to know what should happen
on the other computers. This parameter defines just that.

* `delete_mode=ignore` (default)

  When marked as `ignore` on a computer, delete events are ignored on that
  computer. This is particularly useful to protect a source computer.

  This is the default mode.

* `delete_mode=apply`

  The `delete_mode` can be set to `apply` in order to accept the deletion
  of files on a system.

  This is used only if source files are expected to be managed including
  deletion or renaming. For any other setup, the default, `ignore`, is
  preferable.


