
# Changing Logger Settings

This sub-directory is where administrators create snaplogger settings files
to overwrite the defaults.

The filename is expected to be something like `50-fluid-settings.conf`
where `50` is a priority. Other projects save their defaults either
before (can be overwritten by admins) or after (should not be overwritten
by admins).

To get a list of these filenames, use the `--logger-configuration-filenames`
command line option. Some tools may not support the snaplogger
in which case the tool will not support that command line option.

The generic logger settings are in files named `??-logger.conf`.

The snaprfs tools are in a file that includes the tool's name.
The following are example:

    ??-fluid-settings.conf

where `??` represents a number from 00 to 99. For administrators, it is
preferable to use 50.

