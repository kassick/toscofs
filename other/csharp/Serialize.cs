using System;
using System.Collections;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;

namespace TesteSerialize
{
	class MainClass
	{
		public static void Main (string[] args)
		{
			int nblocks=100;
			BitArray occupied = new BitArray(nblocks, false);
			// ... preenche occupied
			occupied.Set(10, true);
			occupied.Set(2, true);
			for (int i = 50; i < 56; i++)
				occupied.Set(i, true);
			

			MemoryStream ms = new MemoryStream();
			BinaryFormatter BF = new BinaryFormatter();
			BF.Serialize(ms, occupied);
			Byte[] bytes = ms.GetBuffer();
			// bytes contem os dados a serem gravados nos setores

			BinaryWriter Writer = new BinaryWriter(File.OpenWrite("teste.dat"));
			Writer.Write(bytes);
			Writer.Flush();
			Writer.Close();
			Console.WriteLine ("Escreveu!");

			BinaryReader Reader = new BinaryReader(File.OpenRead("teste.dat"));
			// Lê a MESMA quantidade que foi escrita
			Byte[] bytes_in = Reader.ReadBytes(bytes.Length);

			MemoryStream mem_in = new MemoryStream(bytes_in);
			BitArray ba_in = (BitArray)BF.Deserialize(mem_in);

			for (int i = 0; i < nblocks; i++)
				if (ba_in.Get(i) != occupied.Get(i)) {
					Console.WriteLine("Posição {0} não corresponde!", i);
				} else {
					Console.WriteLine("Posição {0} ok -- {1}", i, ba_in.Get(i));
				}
			Reader.Close();
		}
	}
}
