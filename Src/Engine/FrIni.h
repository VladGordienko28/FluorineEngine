/*=============================================================================
    FrIni.h: Config files support class.
    Copyright Oct.2016 Vlad Gordienko.
=============================================================================*/

/*-----------------------------------------------------------------------------
    CIniFile.
-----------------------------------------------------------------------------*/

//
// An ini file class.
//
class CIniFile
{
public:
	// Ini-file constructor.
	CIniFile( String InFileName )
		:	FileName( InFileName ),
			bDirty( false ),
			Sections()
	{
		if( !GPlat->FileExists(FileName) )
			error( L"Ini-File '%s' not found", *FileName );
		LoadFromFile();
	}

	// Ini-file destructor.
	~CIniFile()
	{
		if( bDirty )
			SaveToFile();
	}

	// Save all changes to the file, if they was.
	void Flush()
	{
		if( bDirty )
			SaveToFile();
	}

	// Read bool value.
	Bool ReadBool( const Char* Section, const Char* Key, Bool Default=false )
	{
		String Value = ReadString( Section, Key, L"False" );
		Bool Result = Default;
		if( Value == L"True" )
			Result	= true;
		else
			Result	= _wtoi(*Value) != 0;
		return Result;
	}

	// Read integer value.
	Integer ReadInteger( const Char* Section, const Char* Key, Integer Default=0 )
	{
		String Value = ReadString( Section, Key );
		Integer Result;
		Value.ToInteger( Result, Default );
		return Result;
	}

	// Read float value.
	Float ReadFloat( const Char* Section, const Char* Key, Float Default=0.f )
	{
		String Value = ReadString( Section, Key );
		Float Result;
		Value.ToFloat( Result, Default );
		return Result;
	}

	// Read string value.
	String ReadString( const Char* Section, const Char* Key, String Default=L"" )
	{
		TPair* Pair = FindPair( Section, Key );
		return Pair ? Pair->Value : Default;
	}

	// Write bool value.
	void WriteBool( const Char* Section, const Char* Key, Bool Value )
	{
		WriteString( Section, Key, Value ? L"True" : L"False" );		
	}	
	
	// Write integer value.
	void WriteInteger( const Char* Section, const Char* Key, Integer Value )
	{
		WriteString( Section, Key, String::Format(L"%i", Value) );
	}

	// Write float value.
	void WriteFloat( const Char* Section, const Char* Key, Float Value )
	{
		WriteString( Section, Key, String::Format(L"%.4f", Value) );
	}

	// Write string value.
	void WriteString( const Char* Section, const Char* Key, String Value )
	{
		TPair* Pair = FindPair( Section, Key );
		if( Pair )
		{
			// Override value.
			Pair->Value	= Value;
		}
		else
		{
			// Create new one.
			TSection* Sec = FindSection( Section );

			TPair NewPair;
			NewPair.Key			= Key;
			NewPair.Value		= Value;

			if( Sec )
			{
				// Write to exist section.
				Sec->Pairs.Push( NewPair );
			}
			else
			{
				// Create new section.
				TSection NewSec;
				NewSec.Name	= Section;
				NewSec.Pairs.Push( NewPair );
				Sections.Push( NewSec );
			}
		}
		bDirty	= true;
	}

private:
	// Structures.
	struct TPair
	{
		String			Key;
		String			Value;
	};
	struct TSection
	{
		String			Name;
		TArray<TPair>	Pairs;
	};

	// Ini-file db.
	Bool				bDirty;
	TArray<TSection>	Sections;
	String				FileName;

	// Load file.
	void LoadFromFile()
	{
		CTextReader Reader( FileName );
		TSection* Section = nullptr;
		while( !Reader.IsEOF() )
		{
			String Line = Reader.ReadLine();
			Char *Walk = *Line;
			while( *Walk == ' ' && *Walk )
				Walk++;

			if( *Walk == '['  )
			{
				// Read new section.
				Walk++;
				Char Buffer[64] = {}, *BuffWalk = Buffer;
				while( *Walk && *Walk != ']' )
					*BuffWalk++ = *Walk++;
				TSection NewSec;
				NewSec.Name	= Buffer;
				Sections.Push(NewSec);
				Section = &Sections.Last();
			}
			else if( *Walk != L';' && *Walk != 0  )
			{
				// Read key and value.
				Char KeyBuff[64]={}, ValBuff[64]={}, *KeyWalk=KeyBuff, *ValWalk=ValBuff;
				while( *Walk && *Walk != '=' )
					*KeyWalk++ = *Walk++;
				if( *Walk && *Walk=='=' )
				{
					Walk++;
					while( *Walk && *Walk!=';' )
						*ValWalk++ = *Walk++;
					TPair Pair;
					Pair.Key	= KeyBuff;
					Pair.Value	= ValBuff;
					if( Section )
						Section->Pairs.Push( Pair );
				}
			}
		}
	}

	// Save internal db to file.
	void SaveToFile()
	{
		CTextWriter Writer( FileName );
		for( Integer iSec=0; iSec<Sections.Num(); iSec++ )
		{
			TSection* Section = &Sections[iSec];
			Writer.WriteString(String::Format(L"[%s]", *Section->Name));
			for( Integer iPair=0; iPair<Section->Pairs.Num(); iPair++ )
			{
				TPair* Pair = &Section->Pairs[iPair];
				Writer.WriteString(String::Format(L"%s=%s", *Pair->Key, *Pair->Value ));
			}
			Writer.WriteString(L"");
		}
	}

	// Find section by name, if not found return null.
	TSection* FindSection( const Char* SecName )
	{
		for( Integer i=0; i<Sections.Num(); i++ )
			if( Sections[i].Name == SecName )
				return &Sections[i];
		return nullptr;
	}

	// Find a key pair in section.
	TPair* FindPair( const Char* SecName, const Char* KeyName )
	{
		TSection* Section = FindSection( SecName );
		if( Section )
		{
			for( Integer i=0; i<Section->Pairs.Num(); i++ )
				if( Section->Pairs[i].Key == KeyName )
					return &Section->Pairs[i];
		}
		return nullptr;
	}
};


/*-----------------------------------------------------------------------------
    The End.
-----------------------------------------------------------------------------*/