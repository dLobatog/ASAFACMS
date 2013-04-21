import java.io.*;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder; 

import gnu.getopt.Getopt;


public class Client {
	
	private static boolean _debug = true;
	private static String _server = null;
	private static int _port = -1;
	private static DataOutputStream _out;
	private static DataInputStream _in;
	
	private static final byte PUT_CODE = 100;
	private static final byte GET_CODE = 50;

	
	public String fillString(String string, int limit, char character) {
		if (string.length() >= limit) {
			return string;
		}
		
		StringBuilder builder = new StringBuilder();
		for (int i = string.length(); i < limit; i++) {
			builder.append(character);
		}
		
		return string;
	}
	
	static void f_put(String title, String xLabel, String yLabel, String style, String file_path){
		if (_debug)
			System.out.println("PUT");
		// Send PUT request
		
		byte[] buffer = new byte[101];
		ByteBuffer bb = ByteBuffer.wrap(buffer);
//		bb.order(ByteOrder.LITTLE_ENDIAN);
		bb.put(PUT_CODE);
		bb.put(title.getBytes());
		bb.position(bb.position() + (20 - title.length()));
		bb.put(xLabel.getBytes());
		bb.position(bb.position() + (20 - xLabel.length()));
		bb.put(yLabel.getBytes());
		bb.position(bb.position() + (20 - yLabel.length()));
		bb.put(style.getBytes());
		bb.position(bb.position() + (20 - style.length()));
		bb.put(file_path.getBytes());
		bb.position(bb.position() + (20 - file_path.length()));

		try {
			_out.write(buffer);
			_out.flush();
		} catch (java.io.IOException e) {
			e.printStackTrace();
			return;
		}

		// Send file size
		RandomAccessFile fdin = null;
		try {
			fdin = new RandomAccessFile(file_path, "r");
		} catch (FileNotFoundException e1) {
			e1.printStackTrace();
		}

		try {
			if (fdin.length() == 0)
				System.out.println("c> Input file is empty");
			System.out.println("c> Input file size is " + fdin.length());

			buffer = new byte[10];
			bb = ByteBuffer.wrap(buffer);
//			bb.order(ByteOrder.LITTLE_ENDIAN);

			bb.clear();
			bb.put(String.valueOf(fdin.length()).getBytes());

			_out.write(buffer);
			_out.flush();
		} catch (java.io.IOException e) {
			System.out.println("c> Could not send file size to server");
			e.printStackTrace();
			return;
		}

		// Send file data

		try {
			buffer = new byte[(int) fdin.length()];
			fdin.read(buffer);
		} catch (IOException e) {
			System.out.println("c> Could not read file");
			e.printStackTrace();
			return;
		}

		bb = ByteBuffer.wrap(buffer);
//		bb.order(ByteOrder.LITTLE_ENDIAN);

		bb.clear();
		bb.put(buffer);

		try {
			_out.write(buffer);
			_out.flush();
		} catch (IOException e) {
			System.out.println("c> Could not send file data to server");
			e.printStackTrace();
		}

		// Receive ACK
		buffer = new byte[1];

		bb = ByteBuffer.wrap(buffer);
//		bb.order(ByteOrder.LITTLE_ENDIAN);

		bb.clear();
		try {
			_in.read(buffer);
		} catch (IOException e) {
			System.out.println("c> Did not receive ACK");
			e.printStackTrace();
		}

		if (buffer[0] == PUT_CODE) {
			System.out.println("c> OK");
		} else {
			System.out
					.println("c> ACK failed. Plot might have not been generated");
		}
		
	}

	static void f_get(String dst){
		if (_debug)
			System.out.println("GET");
		
		// Send GET request
		
		byte[] buffer = new byte[31];
		ByteBuffer bb = ByteBuffer.wrap(buffer);
//		bb.order(ByteOrder.LITTLE_ENDIAN);
		bb.put(GET_CODE);
		bb.put(dst.getBytes());

		try {
			_out.write(buffer);
			_out.flush();
		} catch (java.io.IOException e) {
			System.out.println("c> Could not send GET request");
			e.printStackTrace();
			return;
		}
		
		// Read file size

		buffer = new byte[10];

		bb = ByteBuffer.wrap(buffer);
//		bb.order(ByteOrder.LITTLE_ENDIAN);

		bb.clear();
		try {
			_in.readFully(buffer); 
		} catch (IOException e) {
			System.out.println("c> Could not receive file size");
			e.printStackTrace();
			return;
		}
		
		// Read file data
		
		String filesize = null;
	    filesize = new String(buffer);
		System.out.println("c> File size is " + filesize);
		
		buffer = new byte[Integer.parseInt(filesize.replaceAll("[^0-9]",""))];

		bb = ByteBuffer.wrap(buffer);
//		bb.order(ByteOrder.LITTLE_ENDIAN);

		bb.clear();
		try {
			_in.readFully(buffer);
		} catch (IOException e) {
			System.out.println("c> Could not receive file data");
			e.printStackTrace();
			return;
		}
		
		
		// Create and save file
		
		FileOutputStream fos = null;
		
		try {
			fos = new FileOutputStream(dst);
		} catch (FileNotFoundException e) {
            System.out.println("c> Could not download file into local file");
			e.printStackTrace();
			return;
		}
		
		try {
			fos.write(buffer);
			fos.close();
		} catch (IOException e) {
			System.out.println("c> Could not write into local file");
			e.printStackTrace();
		}
		
	}

	static void f_quit(){
		if (_debug)
			System.out.println("QUIT");
	
		// Write code here
	}

	static void usage() {
		System.out.println("Usage: java -cp . client [-d] -s <server> -p <port>");
	}
	
	static boolean parseArguments(String [] argv) {
		Getopt g = new Getopt("client", argv, "ds:p:");

		int c;
		String arg;

		while ((c = g.getopt()) != -1) {
			switch(c) {
				case 'd':
					_debug = true;
					break;
				case 's':
					_server = g.getOptarg();
					break;
				case 'p':
					arg = g.getOptarg();
					_port = Integer.parseInt(arg);
					break;
				case '?':
					System.out.print("getopt() returned " + c + "\n");
					break; // getopt() already printed an error
				default:
					System.out.print("getopt() returned " + c + "\n");
			}
		}
		
		if (_server == null)
			return false;
		
		if ((_port < 1024) || (_port > 65535)) {
			System.out.println("Error: Port must be in the range 1024 <= port <= 65535");
			return false;
		}

		return true;
	}
	
	static void shell() {
		boolean exit = false;
		String [] line;
		BufferedReader in = new BufferedReader(new InputStreamReader(System.in));

		while (!exit) {
			try {
				System.out.print("c> ");
				line = in.readLine().split("\\s");

				if (line.length > 0) {
					if (line[0].equals("put")) {
						if  (line.length == 6)
							f_put(line[1], line[2], line[3], line[4], line[5]);
						else
							System.out.println("Syntax error. Use: put <title> <xLabel> <yLabel> <style> <file_path>");
					} else if (line[0].equals("get")) {
						if  (line.length == 2)
							f_get(line[1]);
						else
							System.out.println("Syntax error. Use: get <graphics>");
					} else if (line[0].equals("quit")){
						if (line.length == 1)
						{
							exit = true;
						} else {
							System.out.println("Syntax error. Use: quit");
						}
					} else {
						System.out.println("Error: command '" + line[0] + "' not valid.");
					}
				}
				
			} catch (java.io.IOException e) {
				System.out.println("Exception: " + e);
				e.printStackTrace();
			}
		}
	}

	public static void main(String[] argv) {
		if (!parseArguments(argv)) {
			usage();
			return;
		}

		if (_debug)
			System.out.println("SERVER: " + _server + " PORT: " + _port);

		try {
			Socket _socket = new Socket(_server, _port);

			_out = new DataOutputStream(_socket.getOutputStream());
			_in = new DataInputStream(_socket.getInputStream());

			shell();

			_out.close();
			_in.close();
			_socket.close();
		} catch (java.io.IOException e) {
			e.printStackTrace();
		}

	}
}
