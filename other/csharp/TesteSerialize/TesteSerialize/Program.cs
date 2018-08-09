using System;
using System.Collections;
using System.Runtime.Serialization.Formatters.Binary;
using System.IO;

namespace TesteSerialize
{
	class Disk {
		FileStream backing_file;
		BinaryReader br;
		BinaryWriter bw;
		public int nsectors;

		public const int SectorSize = 4096;

		public Disk(string fname, int nsectors)
		{
			backing_file = File.Open(fname, FileMode.OpenOrCreate);
			br = new BinaryReader(backing_file);
			bw = new BinaryWriter(backing_file);
			this.nsectors = nsectors;
		}

		public void WriteSector(int sector_nr, Byte[] sector_data)
		{
			if (sector_data.Length > SectorSize) {
				throw new Exception("Só pode escrever em setores!\n");
			} else if (sector_data.Length != SectorSize) {
				Byte[] tmp_data = new Byte[SectorSize];
				Array.Clear(tmp_data, 0, tmp_data.Length);
				Array.Copy(sector_data, tmp_data, sector_data.Length);
				sector_data = tmp_data;
			}

			bw.Seek(sector_nr* SectorSize, SeekOrigin.Begin);
			bw.Write(sector_data);
		}

		public void ReadSector(int sector_nr, Byte[] sector_data)
		{
			// Não lê mais que o buffer comporta
			int read_len = Math.Min(SectorSize, sector_data.Length);
			Array.Clear(sector_data, 0, sector_data.Length);

			br.BaseStream.Seek(sector_nr* SectorSize, SeekOrigin.Begin);
			br.Read(sector_data, 0, read_len);
		}

		public void Dispose() {
			backing_file.Flush();
			backing_file.Close();
		}
	}

	[Serializable()]
	public class SuperBlock {
		public int nblocks; // QUantos blocos tem no meu disco
		public int nblocks_bm; // quantos blocos pro meu bitmap;
		public int block_raiz; // onde está o diretório raiz
	}

	[Serializable()]
	public class Directory {
		int nentries;
		// ...
	}

	class TFS {
		public const int BlockSize = Disk.SectorSize;

		public SuperBlock sb;
		public Directory root_dir;
		public BitArray bm;

		public static void mkfs(Disk d)
		{
			SuperBlock sb = new SuperBlock();
			// Usa o mesmo tamanho de bloco e setor, fica mais fácil aqui
			sb.nblocks = d.nsectors;
			// Uma bitarray para gerenciar os blocos livres, TODOS os blocos do disco
			BitArray bm = new BitArray(sb.nblocks);

			// Precisamos saber quanto vai ocupar o bitmap!
			MemoryStream ms = new MemoryStream();
			BinaryFormatter bf = new BinaryFormatter();
			bf.Serialize(ms, bm);
			int bitmap_blocks = (int)Math.Ceiling((double)ms.GetBuffer().Length / BlockSize);
			Console.WriteLine("Buffer tem {0} bytes", ms.GetBuffer().Length);
			ms.Dispose();

			// Agora Conclui o superblock!
			sb.nblocks_bm = bitmap_blocks;
			sb.block_raiz = sb.nblocks_bm + 1; // PUla o bloco do superblock e pula todos os que o bitmap ocupar

			// Marca os blocos ocupados no bitmap!
			bm.Set(0, true); // superblock ocupado
			bm.Set(sb.block_raiz, true); // diretório raiz ocupado
			for (int i = 0; i < bitmap_blocks; i++)
				bm.Set(i+1, true); // Bloco do bitmap está ocupado tb

			// Agora grava o superblock
			ms = new MemoryStream();
			bf.Serialize(ms, sb);

			d.WriteSector(0, ms.GetBuffer());
			ms.Dispose();


			// Isso poderia estar numa função estática WriteBitmap(Disk, BitArray)
			// Agora grava o bitmap
			ms = new MemoryStream();
			bf.Serialize(ms, bm);

			// O bitmap pode ser vários setores!
			Byte[] buffer = ms.GetBuffer();
			Byte[] sector = new byte[Disk.SectorSize];
			int sector_cursor = 1; // Qual bloco/setor estamos escrevendo
			int cursor = 0;			// Qual pedaço do vetor estamos escrevendo
			int remaining = buffer.Length; // Quantos bytes faltam

			while (remaining > 0) {
				int rem_bytes = Math.Min(remaining, Disk.SectorSize);
				Array.Clear(sector, 0, sector.Length);
				Array.Copy(buffer, cursor, sector, 0, rem_bytes);
				d.WriteSector(sector_cursor, sector);

				sector_cursor ++;
				remaining -= Disk.SectorSize; // pode ficar negativo, mas igual sai do laço
				cursor += Disk.SectorSize; 	  // Pode passar do limite do vetor, mas nesse caso remaining vai ser negativo e a gente sai do laço
			}

			System.Console.WriteLine("Criado sistema de arquivos:\n"+
				"\t{0} blocos\n"+
				"\t{1} blocos de bitmap\n"+
				"\tDiretório raiz em {2}\n",
				sb.nblocks,
				sb.nblocks_bm,
				sb.block_raiz);
		}

		public static TFS mount(Disk d)
		{
			Byte[] block = new Byte[BlockSize];
			d.ReadSector(0, block);

			MemoryStream ms = new MemoryStream(block);
			BinaryFormatter bf = new BinaryFormatter();
			SuperBlock sb = (SuperBlock)bf.Deserialize(ms);

			Byte[] bmbytes = new Byte[sb.nblocks_bm * BlockSize];
			for (int i = 0; i < sb.nblocks_bm; i++)
			{
				d.ReadSector(i+1, block);
				Array.Copy(block, 0, bmbytes, i*BlockSize, BlockSize);
			}

			ms = new MemoryStream(bmbytes);
			BitArray bm = (BitArray)bf.Deserialize(ms);

			if (!bm.Get(0))
				throw new Exception("Bloco 0 não ocupado!\n");
			if (!bm.Get(sb.block_raiz))
				throw new Exception("Diretório raiz não está marcado ocupado\n");

			// lê bloco do diretório raiz

			TFS fs = new TFS();
			fs.sb = sb;
			fs.root_dir = null; // depois associa o diretorio raiz deserializado
			fs.bm = bm;

			Console.WriteLine("Montou um TFS com \n"+
				"\t{0} blocos\n"+
				"\t{1} blocos de bitmap\n"+
				"\tDiretório raiz em {2}\n",
				sb.nblocks,
				sb.nblocks_bm,
				sb.block_raiz);

			return fs;


		}
	}
			
	class MainClass
	{
		public static void Main (string[] args)
		{
			/*
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
			*/

			// Disco "grande" para ocupar vários blocos de bitmap
			// 200M -> 2 blocos de bitmap
			Disk d = new Disk("teste.dat", 200*1000*1000/Disk.SectorSize);
			TFS.mkfs(d);

			TFS fs = TFS.mount(d);

			d.Dispose();

		}
	}
}
