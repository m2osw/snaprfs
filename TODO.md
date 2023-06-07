
# Core Implementation (SNAP-658)

* TCP data connection (for plain & encrypted transmissions)
* Compress the file before sending unless under X bytes
* Make sure watch-dirs parent/child + recursive do not overlap
* Make sure all temporary directories are not included in watch-dirs

# Extensions

* Support to keep files in memory (i.e. cache)
* UDP data connection (for non-encrypted transmissions in broadcast mode)
* Add a timeout on our TCP data connection so if receiving data is too slow
  or does not really happen, we don't keep the connection open (this should
  be something in our eventdispatcher)
* Start only after mount is done (TBD possible with systemctl?)

# Bonuses

* Count events for stats purposes (i.e. how many reads/writes/etc.)
* REST API (Requirement: HTTPD implementation in edhttp SNAP-695)
* Warning on unrecognized options in watch-dirs .conf files (so that way the
  administrator knows something is misspelled).
* Implement a set of system defaults such as copying the /var/crash from any
  computer to one "central" computer and the /var/log too.
* Implement a library to allow for ad-hoc transfers
* Command line `rfs cp|list|version|...` to in part test that the copy works

