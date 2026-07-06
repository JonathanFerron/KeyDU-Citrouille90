#include "usb_ep_stream.h"
#include "usb_ctrl.h"    /* usb_ctrl_ep_size, usb_ctrl_req, usb_device_state */
#include <avr/pgmspace.h>

ep_ready_result_t ep_write_stream(const void* buf, uint16_t length,
                                  uint16_t* bytes_done)
{ uint16_t       done = bytes_done ? *bytes_done : 0;
  const uint8_t* p    = (const uint8_t*)buf + done;
  ep_ready_result_t err = ep_wait_ready();
  if(err) return err;

  while(done < length)
  { if(!ep_rw_allowed())
    { ep_clear_in();
      if(bytes_done)
      { *bytes_done = done;
        return EP_READY_OK;
      }
      err = ep_wait_ready();
      if(err) return err;
    }
    else
    { ep_write_u8(*p++);
      done++;
    }
  }
  return EP_READY_OK;
}

/* --- Control endpoint stream helpers --- */
ep_ready_result_t ep_write_ctrl_stream_P(const void* buf, uint16_t length)
{ //PORTF.OUTTGL = PIN2_bm;  // quick pulse
  //PORTF.OUTTGL = PIN2_bm;

  const uint8_t* p   = (const uint8_t*)buf;
  uint16_t       rem = length;

  if(rem > usb_ctrl_req.w_length)
    rem = usb_ctrl_req.w_length;

  while(rem)
  { /* Wait for endpoint ready.
                Iteration 1: BUSNAK=1 from ep_configure_prv → immediate.
                Iteration 2+: hardware sets BUSNAK=1 when MCNT=CNT (batch done). */
    while(!ep_in_ready())
    { if(usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
      if(ep_setup_received())                      return EP_READY_OK;
    }

    /* Load entire batch (up to 64 B) into the FIFO buffer. */
    uint8_t batch = (rem > (uint16_t)EP_MAX_SIZE) ? (uint8_t)EP_MAX_SIZE : (uint8_t)rem;
    for(uint8_t i = 0; i < batch; i++)
      usb_ep_fifo->data[i] = pgm_read_byte(p++);

    /* Commit: CNT = batch, MCNT = 0.  MULTIPKT sends in BUFSIZE-byte
                sub-packets with auto-toggle until MCNT == CNT, then BUSNAK=1. */
    usb_ep_fifo->position = batch;
    ep_clear_in();

    rem -= batch;
    /* Do NOT wait here for batch completion:
                - If rem > 0: next ep_in_ready() call blocks until BUSNAK=1.
                - If rem == 0: ep_complete_ctrl_status() naturally waits for the
                  host STATUS ZLP, which only arrives after all IN data is received. */
  }

  // led toggle test
  // for(uint8_t i = 0; i < 3; i++)
  // { PORTF.OUTTGL = PIN2_bm;
  //   for(volatile uint16_t d = 0; d < 24000u; d++) {}
  //   PORTF.OUTTGL = PIN2_bm;
  //   for(volatile uint16_t d = 0; d < 24000u; d++) {}
  // }

  /* ZLP when total is an exact multiple of the control EP packet size. */
  if(length && (length % usb_ctrl_ep_size == 0))
  { while(!ep_in_ready())
    { if(usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
    }
    usb_ep_fifo->position = 0;
    ep_clear_in();
  }

  return EP_READY_OK;
}

ep_ready_result_t ep_write_ctrl_stream(const void* buf, uint16_t length)
{ const uint8_t* p   = (const uint8_t*)buf;
  uint16_t       rem = length;

  if(rem > usb_ctrl_req.w_length)
    rem = usb_ctrl_req.w_length;

  while(rem)
  { while(!ep_in_ready())
    { if(usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
      if(ep_setup_received())                      return EP_READY_OK;
    }

    uint8_t batch = (rem > (uint16_t)EP_MAX_SIZE) ? (uint8_t)EP_MAX_SIZE : (uint8_t)rem;
    for(uint8_t i = 0; i < batch; i++)
      usb_ep_fifo->data[i] = *p++;

    usb_ep_fifo->position = batch;
    ep_clear_in();

    rem -= batch;
  }

  if(length && (length % usb_ctrl_ep_size == 0))
  { while(!ep_in_ready())
    { if(usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
    }
    usb_ep_fifo->position = 0;
    ep_clear_in();
  }

  return EP_READY_OK;
}

ep_ready_result_t ep_read_ctrl_stream(void* buf, uint16_t length)
{ uint8_t*  p   = (uint8_t*)buf;
  uint16_t  rem = length;

  while(rem)
  { while(!ep_out_received())
    { if(usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
      if(ep_setup_received())                      return EP_READY_OK;
    }

    while(rem && ep_rw_allowed())
    { *p++ = ep_read_u8();
      rem--;
    }
    ep_clear_out();
  }

  return EP_READY_OK;
}
