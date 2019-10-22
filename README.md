
<p align="center">
<img alt="snaprfs" title="Cluster File  Replicator."
src="https://raw.githubusercontent.com/m2osw/snaprfs/master/doc/snaprfs-logo.png" width="70" height="70"/>
</p>

# Introduction

The `snaprfs` service is a remote file system used to seemlessly duplicate
files between your cluster computers.

## Features

The following are the main features of this file system:

* Access: TCP messages/transfers, UDP, REST API

    The communication between `snaprfs` instances and clients can done
    through various means:
    
    - TCP messages: we reuse the message class from our snapcommunicator

        This is quite useful and very efficient for internal systems
        as this message scheme is very light. The TCP message channel
        is only used to send and receive control messages. The transfers
        are performed on separate channels.

    - TCP transfer

        Whenever a file gets transferred we can send it over TCP. This is
        only used when the file needs to be transferred encrypted. For
        example, we allow the users to upload their SSH public key which
        we want to install on all machines. This feature allows us to
        make such transferred safer.

        Any client data should also be transferred encrypted (i.e. phone
        numbers, addresses, etc.)

    - UDP transfer

        The UDP port can be used to transfer files quickly over UDP by
        sending the files to multiple computers in one go. The switches
        between the computers will be responsible for duplicating the
        data.

        Ultimately, all the listeners should make use of a group setup
        so we can send files to a group of listeners and not to all
        the computers in your cluster. For example, a setting that is
        only useful to the Cassandra database only needs to be sent to
        the Cassandra computers.

        TBD: how do we define such groups automatically?

    - REST API over HTTP

        In order to allow for many more tools to support our system,
        we want to have a full REST API to access the functionality
        offerred by `snaprfs`.

        The REST API is much heavier though: (1) it requires a full HTTP
        header with the correct this and that; (2) it requires that the
        data be transferred over HTTP and not UDP so replication can't
        happen automatically at the switch level; (3) security wise, this
        is likely accessible over the Internet...

* Setup directories or files to be duplicated automatically

    The snaprfs detects files being modified, created, and deleted on any
    system it is running on. It can be setup to automatically copy those
    files between your cluster computers.

* Cluster

    We pretty much want all the computers in our cluster to run `snaprfs`.
    This means we'll have a cluster of `snaprfs` and we want them all to
    know of each others for several reasons:

    - whenever a file needs to be copied to many computers, we need to
      know of them

    - to make use of the `snaprfs` as a distributed cache system, we need
      to know which one needs to be accessed to get the cached data; this
      allows us to send ONE request to see whether the data is available
      or not; if not, we'll load said data

    This code will certainly somewhat replicate what we have in
    snapcommunicator, but the code is going to be very specialized
    for the `snaprfs` functionality.

* Cache

    It is possible to ask `snaprfs` to keep a copy of your files in memory.
    This allows for a very fast cache. Each file is indexed by its path as
    its key.

    If you know of Redis, this is very similar, but instead of just values,
    in our case we cache files (of course, you could use a very small value
    such as an integer, but that's not the primary intend of `snaprfs`).

    Files can further be marked as "memory only worthy". This allows us to
    cache files with a _very small TTL_ without the need to waste time
    saving the file to disk.

    On a reboot, some files can be marked as _auto-reload_. This means they
    get cached in memory immediately, even before they ever get retrieved by
    a client. This is useful so even the very first client does not have
    to wait extra to get that data.

* Global Configurations

    The `snaprfs` has a setup expected to be used to create what we see
    as a global configuration scheme.

    The `advgetopt` project reads configuration files from two locations:

    - /etc/snapwebsites/\<name>.conf
    - /etc/snapwebsites/snapwebsites.d/##-\<name>.conf

    The second location allows us to have any number of configuration files.
    Particularly, it allows us to insert additional entries from different
    sources. One of those sources will ne the `snaprfs` tool which will
    handle files with priority 80 (i.e. `80-<name>.conf`). These files
    are viewed as the global setup.
    
    As a result, you can setup configuration files on a single computer
    and it will automatically propagate to all the others through the
    `snaprfs` file duplication mechanism.

* Auto-detect updates

    The system is implemented to automatically detect when a file changes
    or is created/deleted and can then reflect that change on all the
    other systems.

    In most cases, this is simple because we will use one source computer.
    So changes affecting that source computer get replicated. Other changes
    on other computers are ignored (or rather initiate a "reload correct
    version of the file" feature).

    Whenever a change to a file has to be tracked on any and all computers,
    then that's more complicated because we need to check which change
    happened last and replicate that. This is especially complicated in
    the sense that when you apply a change it triggers a file has changed
    event... (i.e. we need to distinguish between changes from the outside
    and our own changes to replicate a change that happened on another
    computer.)

* Parallelism

    The whole tool is built to work in a multi-threaded environment. The
    idea is to make the tasks concurrent by using workers and not one
    thread per task.

    There are a few specialized threads for other tasks such as listening
    for new connections and communicating with clients.


## The Library

The project comes with a library allowing other projects to easilly make
use of the existing functionality.

The copying of files, though, should be setup in configuration files, not
by using the library. However, there are cases when we want to send a file
dynamically, especially when using the `snaprfs` service as a caching
system.

## Command Line Tools

The project comes with a few command line tools one can use to process files
from the command line. For example, you can quickly copy files between
computers by using the `rfscp ...` utility.

### Copying Files

The `rfscp` tool allows you to copy files without having to create a
corresponding configuration file. This is, of course, a one time thing.
Still very practical if you know you won't need a permanent copy feature
for said file.

### Cluster Info

The `rfsinfo` tool gives you a list of the servers currently connected.
It may also detect that none are available on that computer.

By controlling the output with one row per server we can create a `top`
like tool using `watch -n 1 rfsinfo`.

### Cache Control

The `rfscache` command line tool gives us control over the live cache.
We can reduce it, enlarge it, list the files currently in the cache,
flush the cache out completely, force a reorganization (i.e. if some
files are cached on this computer but should be cached on a different
one, then we can force the transfer of that cached file.)

Whenever a file is requested, the `snaprfs` will first check the memory
cache.

### Stats

The `rfsstats` tool outputs current statistics about the running `snaprfs`
service.

**TBD:** we may want to save our statistics to file so over time we can see
what is used the most, the least, 


# License

The project is covered by the
[GPL 2.0 license](https://github.com/m2osw/snaprfs/blob/master/LICENSE.txt).


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/snaprfs/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et
