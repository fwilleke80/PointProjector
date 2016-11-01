CONTAINER oProjector
{
	NAME oProjector;
	INCLUDE Obase;

	GROUP	PROJECTOR_GROUP_PARAMS
	{
		DEFAULT 1;
		
		LINK		PROJECTOR_LINK				{ ACCEPT { Opolygon; } }
	}
}
