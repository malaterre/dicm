#pragma once

enum error {
  /** Return upon function success */
  kSuccess = 0,
  /** Generic error */
  kError = -1,
  /** DataElement have been sent in out of order */
  kOutOfOrder = -2,
  /** Value Representation if non-ASCII uppercase (A-Z only)*/
  kInvalidVR = -3
};


