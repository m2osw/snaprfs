
# Core Implementation (SNAP-658)

* Create a library to handle the feat
* Control connection (using communicatord)
** The settings (neighbors, etc.) are passed using fluid-settings
** Start file transmission; here is where we send the file metadata
* TCP data connection (for encrypted transmissions)
* UDP data connection (for non-encrypted transmissions)
* Always try to compress the file before sharing (unless under X bytes)
* Copy files from one computer to one or more (i.e. one way)
* Replicate files from any computer to any other (i.e. keep latest)
* Create binary packages
* Support to keep files in memory (i.e. cache)
* Command line `rfs cp|list|version|...` to in part test that the copy works

# Bonuses

* Generate events on file changes
* Count orders for stats purposes (i.e. how many reads/writes/etc.)
* REST API (Requirement: HTTPD implementation in edhttp SNAP-695)
* "Global settings" (duplicate `80-<name>.conf` files; dependency advgetopt,
  see SNAP-690--we may also want to duplicate `20-<name>.conf` so we have
  globals on both sides of the admin file)
* Implement a set of system defaults such as copying the /var/crash from any
  computer to one "central" computer and the /var/log too.

