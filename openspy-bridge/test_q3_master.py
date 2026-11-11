import struct
import unittest

from q3_master import CONNECTIONLESS, Directory, MasterProtocol, RESPONSE_PREFIX


class DirectoryTests(unittest.TestCase):
    def test_heartbeat_and_native_server_list(self):
        directory = Directory()
        self.assertTrue(directory.heartbeat(("192.0.2.10", 27960), "Quake3", now=10))
        packets = directory.list_packet("Quake3", 68, now=11)
        self.assertEqual(len(packets), 1)
        self.assertTrue(packets[0].startswith(RESPONSE_PREFIX))
        self.assertIn(bytes((192, 0, 2, 10)) + struct.pack("!H", 27960), packets[0])
        self.assertTrue(packets[0].endswith(b"\\EOT"))

    def test_expiration_and_shutdown(self):
        directory = Directory(ttl=30)
        directory.heartbeat(("192.0.2.10", 27960), "Quake3", now=10)
        self.assertEqual(len(directory.list_packet("Quake3", 68, now=41)), 1)
        self.assertTrue(directory.list_packet("Quake3", 68, now=41)[0].endswith(b"\\EOT"))
        directory.heartbeat(("192.0.2.11", 27960), "Quake3", now=50)
        directory.shutdown(("192.0.2.11", 27960))
        self.assertEqual(directory.servers, {})

    def test_rejects_unknown_game_and_protocol(self):
        directory = Directory()
        self.assertFalse(directory.heartbeat(("192.0.2.10", 27960), "not-quake", now=1))
        self.assertTrue(directory.list_packet("Quake3", 999, now=2)[0].endswith(b"\\EOT"))
        self.assertTrue(directory.list_packet("not-quake", 68, now=2)[0].endswith(b"\\EOT"))

    def test_ipv4_only_and_port_validation(self):
        directory = Directory()
        self.assertFalse(directory.heartbeat(("2001:db8::1", 27960), "Quake3", now=1))
        self.assertFalse(directory.heartbeat(("192.0.2.10", 0), "Quake3", now=1))


if __name__ == "__main__":
    unittest.main()
