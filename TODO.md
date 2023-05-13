
# Core Implementation (SNAP-658)

* TCP data connection (for encrypted transmissions)
* UDP data connection (for non-encrypted transmissions in broadcast mode)
* Compress the file before sending unless under X bytes
* Replicate files from any computer to any other (i.e. keep latest)
* Make sure watch-dirs parent/child + recursive do not overlap
* Make sure all parts directories are not included in watch-dirs
* Start only after mount is done (TBD possible with systemctl?)

# Extensions

* Support to keep files in memory (i.e. cache)
* Implement a library to allow for ad-hoc transfers
* Command line `rfs cp|list|version|...` to in part test that the copy works
* Add a timeout on our TCP data connection so if receiving data is too slow
  or does not really happen, we don't keep the connection open

# Bonuses

* Generate events on file changes
* Count orders for stats purposes (i.e. how many reads/writes/etc.)
* REST API (Requirement: HTTPD implementation in edhttp SNAP-695)
* Warning on unrecognized options in watch-dirs .conf files (so that way the
  administrator knows something is misspelled).
* "Global settings" (duplicate `80-<name>.conf` files; dependency advgetopt,
  see SNAP-690--we may also want to duplicate `20-<name>.conf` so we have
  globals on both sides of the admin file) [I think this is void by the
  fluid-settings]
* Implement a set of system defaults such as copying the /var/crash from any
  computer to one "central" computer and the /var/log too.

