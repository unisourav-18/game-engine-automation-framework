using System;
using System.Diagnostics;
using System.IO;
using System.IO.Pipes;
using System.Text;
using System.Threading;

namespace AutomationTool
{
    class Program
    {
        private const string PipeName = "GameAutomationPipe";

        static int Main(string[] args)
        {
            Console.ForegroundColor = ConsoleColor.Cyan;
            Console.WriteLine("==================================================");
            Console.WriteLine("  UBISOFT DTEST-STYLE AUTOMATED TEST RUNNER (C#)  ");
            Console.WriteLine("==================================================\n");
            Console.ResetColor();

            int testsPassed = 0;
            int totalTests = 3;

            // --- TEST 1: Ping / Spawn Verification ---
            Console.Write("[RUNNING] Test 1: Spawning Player Entity via IPC... ");
            if (SendCommand("SPAWN_PLAYER"))
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("PASS");
                Console.ResetColor();
                testsPassed++;
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("FAIL (IPC Timeout)");
                Console.ResetColor();
            }

            Thread.Sleep(500);

            // --- TEST 2: Stress / Multi-entity Allocation ---
            Console.Write("[RUNNING] Test 2: Stress Testing Entity Allocation (x5)... ");
            bool stressPassed = true;
            for (int i = 0; i < 5; i++)
            {
                if (!SendCommand("SPAWN_PLAYER"))
                {
                    stressPassed = false;
                    break;
                }
                Thread.Sleep(100);
            }

            if (stressPassed)
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("PASS");
                Console.ResetColor();
                testsPassed++;
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("FAIL");
                Console.ResetColor();
            }

            Thread.Sleep(500);

            // --- TEST 3: Graceful Engine Shutdown ---
            Console.Write("[RUNNING] Test 3: Engine Clean Shutdown Command... ");
            if (SendCommand("SHUTDOWN"))
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine("PASS");
                Console.ResetColor();
                testsPassed++;
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("FAIL");
                Console.ResetColor();
            }

            // --- SUMMARY REPORT ---
            Console.WriteLine("\n--------------------------------------------------");
            if (testsPassed == totalTests)
            {
                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine($"TEST SUITE COMPLETED: {testsPassed}/{totalTests} PASSED (ALL PASS)");
                Console.ResetColor();
                return 0; // Return code 0 for CI/CD success
            }
            else
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"TEST SUITE FAILED: {testsPassed}/{totalTests} PASSED");
                Console.ResetColor();
                return 1; // Non-zero exit code for CI/CD failure
            }
        }

        private static bool SendCommand(string command)
        {
            try
            {
                using (var pipeClient = new NamedPipeClientStream(".", PipeName, PipeDirection.Out))
                {
                    pipeClient.Connect(2000); // 2 sec timeout
                    byte[] buffer = Encoding.UTF8.GetBytes(command);
                    pipeClient.Write(buffer, 0, buffer.Length);
                    pipeClient.Flush();
                }
                return true;
            }
            catch
            {
                return false;
            }
        }
    }
}