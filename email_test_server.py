#!/bin/python3
import smtpd, builtins, asyncore, os, ctypes, sys

PID_FILE='/build/email_mock.pid'
def _():
  # Check pid, if its still running, then we just exit
  try:
    pid_file = open(PID_FILE)
  except FileNotFoundError:
    return
  with pid_file:
    pid = int(pid_file.read())
    try:
      os.kill(pid, 0)
    except OSError:
      return
  exit(0)
_()

print("Starting mock smtp server")

# Set process title
ctypes.CDLL('', use_errno=True).prctl(15, b"email_mock", 0,0,0)


class Server(smtpd.SMTPServer):
  def process_message(self, peer, mailfrom, rcpttos, data, **kwargs):
    out_file_path = '/build/email_mock_' + '_'.join(rcpttos)
    with open(out_file_path, 'a') as out_file:
      def print(*a, **kw):
        kw['file'] = out_file
        builtins.print(*a, **kw)
      print(f'===> MAIL FROM:<{mailfrom}>')
      for to in rcpttos:
        print(f'===> RCPT TO:<{to}>')
      print('---------- MESSAGE FOLLOWS ----------')
      if kwargs:
          if kwargs.get('mail_options'):
              print('mail options: %s' % kwargs['mail_options'])
          if kwargs.get('rcpt_options'):
              print('rcpt options: %s\n' % kwargs['rcpt_options'])
      inheaders = 1
      lines = data.splitlines()
      for line in lines:
          # headers first
          if inheaders and not line:
              peerheader = 'X-Peer: ' + peer[0]
              if not isinstance(data, str):
                  # decoded_data=false; make header match other binary output
                  peerheader = repr(peerheader.encode('utf-8'))
              print(peerheader)
              inheaders = 0
          if not isinstance(data, str):
              # Avoid spurious 'str on bytes instance' warning.
              line = repr(line)
          print(line)
      print('------------ END MESSAGE ------------')

proxy = Server(('0.0.0.0', 8025), None, 1<<20, enable_SMTPUTF8=True)

sys.stdout.flush()
sys.stderr.flush()
os.setsid()
pid = os.fork()

if pid:
  with open(PID_FILE, 'w') as pid_file:
    pid_file.write(str(pid))
  print("exiting...")
  os.close(0)
  os.close(1)
  os.close(2)
  exit();
else:
  os.close(0)
  os.close(1)
  os.close(2)
  sys.stderr = sys.stdout = open('/build/email_mock.log', 'a');
  print('running')
  sys.stderr.write('running2\n')
  sys.stderr.flush()
  asyncore.loop()

