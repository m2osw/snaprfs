
# Dependencies

* snapcommunicator in eventdispatcher (probably renamed just communicator
  or maybe use a different much shorter name such as hubd--hub deamon)

# Core Implementation (SNAP-658)

* Create a library to handle the feat
* Control connection (using the snap communicator (event dispatcher) message)
** All the settings (neighbors, etc.) are passed through this connection
** Start file transmission; here is where we send the file metadata
* TCP data connection (for encrypted transmissions)
* UDP data connection (for non-encrypted transmissions)
* Copy files from one computer to one or more (i.e. one way)
* Replicate files from any computer to any other (i.e. keep latest)
* Create binary packages
* Support to keep files in memory (i.e. cache)
* `rfs cp|list|version|...` to in part test that the copy works

# Bonuses

* Generate events on file changes
* Count orders for stats purposes (i.e. how many reads/writes/etc.)
* REST API (Requirement: HTTPD implementation in eventdispatcher SNAP-695)
* Global settings (duplicate 80-<name>.conf files; dependency advgetopt,
  see SNAP-690)
* Implement a set of system defaults such as copying the /var/crash from any
  computer to one "central" computer and the /var/log too.

